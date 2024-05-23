#include "muusbolla/muusbolla.grpc.pb.h"

#include <agrpc/asioGrpc.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>

#include <iostream>

namespace asio = boost::asio;

asio::awaitable<void> make_request(agrpc::GrpcContext& grpc_context, muusbolla::Example::Stub& stub)
{
    grpc::ClientContext client_context;
    std::unique_ptr<grpc::ClientAsyncReaderWriter<muusbolla::Request, muusbolla::Response>> reader_writer;
    co_await agrpc::request(&muusbolla::Example::Stub::AsyncStream, stub, client_context, reader_writer);

    muusbolla::Request request;
    for (double i = 1; i < 8; ++i)
    {
        request.set_input(i);
        std::cout << "Writing: " << i << std::endl;
        co_await agrpc::write(*reader_writer, request);
    }
    co_await agrpc::writes_done(*reader_writer);

    muusbolla::Response response;
    while (co_await agrpc::read(*reader_writer, response))
    {
        std::cout << "Read: " << response.output() << std::endl;
    }

    grpc::Status status;
    co_await agrpc::finish(*reader_writer, status);

    std::cout << "Received finish: " << status.error_code() << std::endl;
}

int main(int argc, const char** argv)
{
    const auto port = argc >= 2 ? argv[1] : "50051";
    const auto host = std::string{"localhost:"} + port;

    agrpc::GrpcContext grpc_context{std::make_unique<grpc::CompletionQueue>()};

    const auto channel = grpc::CreateChannel(host, grpc::InsecureChannelCredentials());
    channel->WaitForConnected(std::chrono::system_clock::now() + std::chrono::seconds{10});

    const auto stub = muusbolla::Example::NewStub(channel);

    asio::co_spawn(
        grpc_context,
        [&]() -> asio::awaitable<void>
        {
            co_await make_request(grpc_context, *stub);
        },
        asio::detached);

    grpc_context.run();
}