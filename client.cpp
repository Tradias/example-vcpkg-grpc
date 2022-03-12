#include "helloworld/helloworld.grpc.pb.h"

#include <agrpc/asioGrpc.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>

#include <syncstream>
#include <thread>
#include <vector>

struct Context
{
    grpc::ClientContext client_context;
    helloworld::HelloRequest request;
    helloworld::HelloReply response;
    grpc::Status status;
    std::unique_ptr<grpc::ClientAsyncResponseReader<helloworld::HelloReply>> reader;
};

void make_requests(agrpc::GrpcContext& grpc_context, helloworld::Greeter::Stub& stub, std::int64_t& requests,
                   std::int64_t& successes, std::atomic_bool& ok)
{
    ++requests;
    auto context = std::allocate_shared<Context>(grpc_context.get_allocator());
    auto& reader = context->reader;
    context->request.set_name("world");
    reader = stub.AsyncSayHello(&context->client_context, context->request, agrpc::get_completion_queue(grpc_context));
    auto& response = context->response;
    auto& status = context->status;
    agrpc::finish(*reader, response, status,
                  boost::asio::bind_executor(grpc_context,
                                             [&, c = std::move(context)](bool)
                                             {
                                                 successes += int{c->status.ok()};
                                                 if (ok.load(std::memory_order_relaxed))
                                                 {
                                                     make_requests(grpc_context, stub, requests, successes, ok);
                                                 }
                                             }));
}

int main(int argc, const char** argv)
{
    const auto thread_count = argc >= 2 ? std::stoi(argv[1]) : 1;
    const std::chrono::seconds runtime_seconds{argc >= 3 ? std::stoi(argv[2]) : 6};
    std::string host{"localhost:50051"};

    boost::asio::io_context io_context{1};
    boost::asio::basic_signal_set signals{io_context.get_executor(), SIGINT, SIGTERM};
    std::atomic_bool ok{true};
    signals.async_wait(
        [&](auto&&, auto&&)
        {
            ok = false;
        });

    // Channel arguments
    grpc::ChannelArguments args{};
    args.SetInt(GRPC_ARG_MINIMAL_STACK, 1);

    const auto channel = grpc::CreateCustomChannel(host, grpc::InsecureChannelCredentials(), args);
    channel->WaitForConnected(std::chrono::system_clock::now() + runtime_seconds);

    const auto stub = helloworld::Greeter::NewStub(channel);

    boost::asio::steady_timer timer{io_context, std::chrono::steady_clock::now() + runtime_seconds};
    timer.async_wait(
        [&](auto&&)
        {
            ok = false;
            signals.cancel();
        });

    std::vector<std::jthread> threads;
    for (size_t i = 0; i < thread_count; ++i)
    {
        threads.emplace_back(
            [&, i]
            {
                agrpc::GrpcContext grpc_context{std::make_unique<grpc::CompletionQueue>()};
                std::int64_t requests{};
                std::int64_t successes{};
                boost::asio::post(grpc_context,
                                  [&]()
                                  {
                                      if (ok.load(std::memory_order_relaxed))
                                      {
                                          make_requests(grpc_context, *stub, requests, successes, ok);
                                      }
                                  });
                const auto start = std::chrono::steady_clock::now();
                grpc_context.run();
                const auto end = std::chrono::steady_clock::now();
                const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                std::osyncstream{std::cout}                                         //
                    << "Thread " << i << " requests: " << requests                  //
                    << " requests/s: " << requests / double(milliseconds) * 1000.0  //
                    << " successes: " << successes                                  //
                    << " successes/s: " << successes / double(milliseconds) * 1000.0 << std::endl;
            });
    }

    io_context.run();
}