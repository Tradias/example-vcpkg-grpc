#include "example/v1/example.grpc.pb.h"
#include "server_shutdown_asio.hpp"

#include <agrpc/asio_grpc.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <grpcpp/create_channel.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

#include <iostream>
#include <thread>

namespace asio = boost::asio;

template <auto PrepareAsync>
using AwaitableClientRPC = boost::asio::use_awaitable_t<>::as_default_on_t<agrpc::ClientRPC<PrepareAsync>>;

int main(int argc, const char** argv)
{
    const auto port = argc >= 2 ? argv[1] : "50051";
    const auto host = std::string("0.0.0.0:") + port;

    // start up Example server
    std::unique_ptr<grpc::Server> server;

    grpc::ServerBuilder builder;
    agrpc::GrpcContext grpc_context{builder.AddCompletionQueue()};
    builder.AddListeningPort(host, grpc::InsecureServerCredentials());
    example::v1::Example::AsyncService service;
    builder.RegisterService(&service);
    server = builder.BuildAndStart();

    // bring in the server shutdown functionality
    example::ServerShutdown server_shutdown{*server, grpc_context};

    // install a request handler for the bidirectional streaming endpoint
    agrpc::repeatedly_request(
        &example::v1::Example::AsyncService::RequestBidirectionalStreaming, service,
        asio::bind_executor(
            grpc_context,
            [&](grpc::ServerContext& context,
                grpc::ServerAsyncReaderWriter<example::v1::Response, example::v1::Request>& reader_writer)
                -> asio::awaitable<void>
            {
                std::cout << "server: started request\n";

                // set up an alarm that is used to pace our responses to the client
                agrpc::Alarm alarm(grpc_context);
                int64_t duration_msec = 1000;

                agrpc::GrpcStream stream{grpc_context};
                example::v1::Request cmd;
                stream.initiate(agrpc::read, reader_writer, cmd);

                while (true)
                {
                    using namespace asio::experimental::awaitable_operators;

                    // wait for the first of two events to happen:
                    //
                    // 1. we receive a new streaming message from the client
                    // 2. the timer expires
                    //
                    // when the timer expires, we send a message to the client.
                    std::cout << "server: reading/waiting\n";
                    auto result = co_await (stream.next() || alarm.wait(std::chrono::system_clock::now() +
                                                                        std::chrono::milliseconds(duration_msec)));
                    if (server_shutdown.is_shutdown)
                    {
                        std::cout << "server: shutdown recieved\n";
                        break;
                    }
                    else if (result.index() == 0)
                    {
                        if (std::get<0>(result))
                        {
                            std::cout << "server: got streaming message\n";
                            stream.initiate(agrpc::read, reader_writer, cmd);
                        }
                        else
                        {
                            std::cout << "server: read failed\n";
                            break;
                        }
                    }
                    else
                    {
                        std::cout << "server: alarm expired\n";
                        example::v1::Response resp;
                        if (!co_await agrpc::write(reader_writer, resp))
                        {
                            // client disconnected
                            break;
                        }
                    }
                }

                co_await agrpc::finish(reader_writer, grpc::Status::OK);
                co_await stream.cleanup();
            }));

    // set up a client stub for this service
    example::v1::Example::Stub stub(
        grpc::CreateChannel(std::string("localhost:") + port, grpc::InsecureChannelCredentials()));
    // spawn a coroutine that will talk to the bidirectional streaming endpoint
    asio::co_spawn(
        grpc_context,
        [&]() -> asio::awaitable<void>
        {
            // create an RPC to the BidirectionalStreaming interface
            using RPC = AwaitableClientRPC<&example::v1::Example::Stub::PrepareAsyncBidirectionalStreaming>;
            RPC rpc{grpc_context};

            // start the RPC
            std::cout << "client: starting RPC\n";
            if (!co_await rpc.start(stub))
            {
                co_return;
            }

            // send the first streaming message to the server
            std::cout << "client: writing streaming message\n";
            example::v1::Request cmd;
            auto write_ok = co_await rpc.write(cmd);

            // now, just wait forever for streaming responses back from the server until a read fails
            bool read_ok = true;
            while (write_ok && read_ok)
            {
                std::cout << "client: waiting for streaming message\n";
                example::v1::Response resp;
                read_ok = co_await rpc.read(resp);
            }

            std::cout << "client: done: " << (co_await rpc.finish()).error_message() << "\n";
        },
        asio::detached);

    // run the gRPC context thread
    grpc_context.run();

    std::cout << "Shutdown completed\n";
}