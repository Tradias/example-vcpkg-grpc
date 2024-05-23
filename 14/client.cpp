#include "helloworld/helloworld.grpc.pb.h"

#include <agrpc/asioGrpc.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>

#include <atomic>
#include <forward_list>
#include <syncstream>
#include <thread>
#include <vector>

namespace asio = boost::asio;

struct Context
{
    grpc::ClientContext client_context;
    helloworld::HelloRequest request;
    helloworld::HelloReply response;
    grpc::Status status;
    std::unique_ptr<grpc::ClientAsyncResponseReader<helloworld::HelloReply>> reader;
};

struct ChannelAndStub
{
    std::shared_ptr<grpc::Channel> channel;
    std::unique_ptr<helloworld::Greeter::Stub> stub;
};

template <class Iterator>
class RoundRobin
{
  public:
    template <class Range>
    explicit RoundRobin(Range&& range)
        : begin(std::begin(range)), size(std::distance(std::begin(range), std::end(range)))
    {
    }

    decltype(auto) next()
    {
        const auto cur = current.fetch_add(1, std::memory_order_relaxed);
        const auto pos = cur % size;
        return *std::next(begin, pos);
    }

  private:
    Iterator begin;
    std::size_t size;
    std::atomic_size_t current{};
};

auto create_channels(const std::string& host, int count)
{
    std::vector<ChannelAndStub> channels;
    for (int i = 0; i < count; ++i)
    {
        grpc::ChannelArguments args{};
        args.SetInt("channel_id", i);
        const auto channel = grpc::CreateCustomChannel(host, grpc::InsecureChannelCredentials(), args);
        channels.emplace_back(channel, helloworld::Greeter::NewStub(channel));
    }
    return channels;
}

auto create_grpc_contexts(std::size_t count)
{
    std::forward_list<agrpc::GrpcContext> grpc_contexts;
    for (std::size_t i = 0; i < count; ++i)
    {
        grpc_contexts.emplace_front(std::make_unique<grpc::CompletionQueue>());
    }
    return grpc_contexts;
}

template <class ExecutionContexts>
auto create_work_guards(ExecutionContexts&& execution_contexts)
{
    std::vector<asio::executor_work_guard<
        asio::associated_executor_t<typename std::remove_cvref_t<ExecutionContexts>::value_type>>>
        guards;
    for (auto&& context : execution_contexts)
    {
        guards.emplace_back(context.get_executor());
    }
    return guards;
}

asio::awaitable<void> make_request(agrpc::GrpcContext& grpc_context, helloworld::Greeter::Stub& stub)
{
    grpc::ClientContext client_context;
    helloworld::HelloRequest request;
    request.set_name("world");
    const auto reader = stub.AsyncSayHello(&client_context, request, agrpc::get_completion_queue(grpc_context));
    helloworld::HelloReply response;
    grpc::Status status;
    co_await agrpc::finish(*reader, response, status);
}

int main(int argc, const char** argv)
{
    const auto thread_count = argc >= 2 ? std::stoi(argv[1]) : 3;
    const auto requests = argc >= 3 ? std::stoi(argv[2]) : 10000000;
    const auto channel_count = argc >= 4 ? std::stoi(argv[3]) : 1;
    std::string host{"localhost:50051"};

    auto channels = create_channels(host, channel_count);
    for (auto&& channel : channels)
    {
        channel.channel->WaitForConnected(std::chrono::system_clock::now() + std::chrono::seconds(5));
    }
    RoundRobin<decltype(channels)::iterator> round_robin_channels{channels};

    auto grpc_contexts = create_grpc_contexts(thread_count);
    RoundRobin<decltype(grpc_contexts)::iterator> round_robin_grpc_contexts{grpc_contexts};

    auto grpc_context_work_guards = create_work_guards(grpc_contexts);

    std::vector<std::thread> grpc_context_threads;
    for (size_t i = 0; i < thread_count; ++i)
    {
        grpc_context_threads.emplace_back(
            [&, i]
            {
                auto& grpc_context = *std::next(grpc_contexts.begin(), i);
                grpc_context.run();
            });
    }

    const auto start = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < requests; ++i)
    {
        auto&& grpc_context = round_robin_grpc_contexts.next();
        auto&& [channel, stub] = round_robin_channels.next();
        asio::co_spawn(
            grpc_context,
            [&]() -> asio::awaitable<void>
            {
                co_await make_request(grpc_context, *stub);
            },
            asio::detached);
    }
    for (auto&& guard : grpc_context_work_guards)
    {
        guard.reset();
    }
    for (auto&& thread : grpc_context_threads)
    {
        thread.join();
    }
    const auto end = std::chrono::steady_clock::now();
    const auto milliseconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << "Time: " << milliseconds << "micros" << std::endl;
}