#include "muusbolla/muusbolla.grpc.pb.h"

#include <agrpc/asioGrpc.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/thread_pool.hpp>
#include <grpcpp/server_builder.h>

#include <iostream>
#include <syncstream>

namespace asio = boost::asio;

using Channel = asio::experimental::channel<void(boost::system::error_code, muusbolla::Request)>;

asio::awaitable<void> reader(grpc::ServerAsyncReaderWriter<muusbolla::Response, muusbolla::Request>& reader_writer,
                             Channel& channel)
{
    bool ok{true};
    while (ok)
    {
        std::osyncstream{std::cout} << "thread " << std::this_thread::get_id() << ": Reading ..." << std::endl;

        muusbolla::Request request;
        ok = co_await agrpc::read(reader_writer, request);

        if (!ok)
        {
            // Client is done writing.
            break;
        }

        std::osyncstream{std::cout} << "thread " << std::this_thread::get_id() << ": Read: " << request.input()
                                    << std::endl;

        // Send request to writer. Using a callback as completion token since we do not want to wait until the writer
        // has picked up the request.
        channel.async_send(boost::system::error_code{}, std::move(request), [&](const boost::system::error_code&) {});
    }

    std::osyncstream{std::cout} << "thread " << std::this_thread::get_id() << ": Received writes done" << std::endl;
    // Signal the writer to complete.
    channel.close();
}

asio::awaitable<void> writer(grpc::ServerAsyncReaderWriter<muusbolla::Response, muusbolla::Request>& reader_writer,
                             Channel& channel, asio::thread_pool& thread_pool)
{
    bool ok{true};
    while (ok)
    {
        boost::system::error_code ec;
        const auto request = co_await channel.async_receive(asio::redirect_error(asio::use_awaitable, ec));
        if (ec)
        {
            // Channel got closed by the reader.
            std::osyncstream{std::cout} << "thread " << std::this_thread::get_id() << ": Stopped writing" << std::endl;
            break;
        }

        // Switch to the thread_pool.
        co_await asio::dispatch(asio::bind_executor(thread_pool, asio::use_awaitable));

        // Compute the response.
        muusbolla::Response response;
        response.set_output(request.input() * 100);

        std::osyncstream{std::cout} << "thread " << std::this_thread::get_id() << ": Writing: " << response.output()
                                    << std::endl;

        // reader_writer is thread-safe so we can just interact with it from the thread_pool.
        ok = co_await agrpc::write(reader_writer, response);
        // Now we are back on the main thread.

        std::osyncstream{std::cout} << "thread " << std::this_thread::get_id() << ": Wrote: " << response.output()
                                    << std::endl;
    }
}

int main(int argc, const char** argv)
{
    const auto thread_count = argc >= 2 ? std::stoi(argv[1]) : 1;
    std::string host{"localhost:50051"};

    grpc::ServerBuilder builder;
    std::unique_ptr<grpc::Server> server;
    muusbolla::Example::AsyncService service;
    builder.AddListeningPort(std::string{host}, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::vector<std::unique_ptr<grpc::ServerCompletionQueue>> queues;
    agrpc::GrpcContext grpc_context(builder.AddCompletionQueue());
    server = builder.BuildAndStart();

    std::cout << "Server listening on " << host << std::endl;

    asio::thread_pool thread_pool{2};

    asio::co_spawn(
        grpc_context,
        [&]() -> asio::awaitable<void>
        {
            grpc::ServerContext server_context;
            grpc::ServerAsyncReaderWriter<muusbolla::Response, muusbolla::Request> reader_writer{&server_context};
            co_await agrpc::request(&muusbolla::Example::AsyncService::RequestStream, service, server_context,
                                    reader_writer);
            Channel channel{grpc_context};
            using namespace asio::experimental::awaitable_operators;
            co_await(reader(reader_writer, channel) && writer(reader_writer, channel, thread_pool));
            co_await agrpc::finish(reader_writer, grpc::Status::OK);
        },
        asio::detached);

    grpc_context.run();

    std::cout << "Server shutdown" << std::endl;
    server->Shutdown();
}
