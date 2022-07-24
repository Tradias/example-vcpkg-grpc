#include "helloworld/helloworld.grpc.pb.h"

#include <agrpc/asio_grpc.hpp>
#include <bind_completion_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>

#include <iostream>
#include <syncstream>

namespace asio = boost::asio;

asio::awaitable<void> make_request(agrpc::GrpcContext& grpc_context, helloworld::Greeter::Stub& stub,
                                   asio::io_context& io_context)
{
    grpc::ClientContext client_context;
    client_context.set_deadline(std::chrono::system_clock::now());

    helloworld::HelloRequest request;
    auto responder =
        agrpc::request(&helloworld::Greeter::Stub::AsyncSayHello, stub, client_context, request, grpc_context);

    std::osyncstream(std::cout) << "Request on: " << std::this_thread::get_id() << std::endl;

    helloworld::HelloReply reply;
    grpc::Status status;
    co_await agrpc::finish(responder, reply, status,
                           agrpc_ext::CompletionExecutorBinder(io_context.get_executor(), asio::use_awaitable));

    std::osyncstream(std::cout) << "Complete on: " << std::this_thread::get_id() << std::endl;
}

int main(int argc, const char** argv)
{
    const auto port = argc >= 2 ? argv[1] : "50051";
    const auto host = std::string{"localhost:"} + port;

    agrpc::GrpcContext grpc_context{std::make_unique<grpc::CompletionQueue>()};

    helloworld::Greeter::Stub stub(grpc::CreateChannel(host, grpc::InsecureChannelCredentials()));

    asio::io_context io_context;
    auto guard = asio::make_work_guard(io_context);

    asio::co_spawn(
        grpc_context,
        [&]() -> asio::awaitable<void>
        {
            co_await make_request(grpc_context, stub, io_context);
        },
        asio::detached);

    std::jthread io_context_thread{[&]
                                   {
                                       std::osyncstream(std::cout)
                                           << "io_context thread: " << std::this_thread::get_id() << std::endl;
                                       io_context.run();
                                   }};

    std::osyncstream(std::cout) << "grpc_context thread: " << std::this_thread::get_id() << std::endl;
    grpc_context.run();
    guard.reset();
}