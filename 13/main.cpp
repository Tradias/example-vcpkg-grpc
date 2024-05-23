#include "dataProvider.hpp"
#include "ole/v1/dataProvider.grpc.pb.h"

#include <agrpc/asioGrpc.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <grpcpp/create_channel.h>

#include <cassert>
#include <future>
#include <syncstream>
#include <thread>
#include <unordered_map>

namespace asio = boost::asio;

using Marker = std::uint64_t;

class Interface
{
  public:
    virtual ole::v1::Data get_additional_data(ole::v1::Request) = 0;
};

class ConnectedClient : Interface
{
  private:
    using RequestChannel = asio::experimental::channel<void(boost::system::error_code, ole::v1::Request)>;
    using DataPromise = std::promise<ole::v1::Data>;

    agrpc::GrpcContext& grpc_context;
    ole::v1::DataProvider::Stub& stub;
    RequestChannel request_channel;
    Marker last_marker{};
    std::unordered_map<Marker, DataPromise*> data_map;

  public:
    ConnectedClient(agrpc::GrpcContext& grpc_context, ole::v1::DataProvider::Stub& stub)
        : grpc_context(grpc_context), stub(stub), request_channel(grpc_context)
    {
    }

    auto get_executor() { return grpc_context.get_executor(); }

    asio::awaitable<void> write_loop(grpc::ClientAsyncReaderWriter<ole::v1::Request, ole::v1::Data>& reader_writer)
    {
        bool ok{true};
        while (ok)
        {
            const auto request = co_await request_channel.async_receive(asio::use_awaitable);
            ok = co_await agrpc::write(reader_writer, request);
        }
    }

    asio::awaitable<void> read_loop(grpc::ClientAsyncReaderWriter<ole::v1::Request, ole::v1::Data>& reader_writer)
    {
        ole::v1::Data data;
        while (co_await agrpc::read(reader_writer, data))
        {
            const auto found = data_map.find(data.id());
            assert(found != data_map.end());
            found->second->set_value(data);
        }
    }

    asio::awaitable<void> run()
    {
        while (true)
        {
            grpc::ClientContext client_context;
            client_context.set_deadline(std::chrono::system_clock::time_point::max());
            client_context.set_wait_for_ready(true);
            auto [reader_writer, ok] =
                co_await agrpc::request(&ole::v1::DataProvider::Stub::AsyncGet, stub, client_context);
            if (!ok)
            {
                continue;
            }
            using namespace asio::experimental::awaitable_operators;
            co_await(read_loop(*reader_writer) && write_loop(*reader_writer));
        }
    }

    ole::v1::Data get_additional_data(ole::v1::Request request) override
    {
        DataPromise promise;
        auto future = promise.get_future();
        asio::post(get_executor(),
                   [&]()
                   {
                       request.set_id(last_marker);
                       data_map.emplace(last_marker, &promise);
                       ++last_marker;
                       request_channel.async_send(boost::system::error_code{}, std::move(request),
                                                  [&](const boost::system::error_code&) {});
                   });
        return future.get();
    }
};

int main(int argc, const char** argv)
{
    std::string host{"localhost:50051"};

    std::thread data_provider_thread{[&]
                                     {
                                         run_data_provider(host);
                                     }};

    const auto stub = ole::v1::DataProvider::NewStub(grpc::CreateChannel(host, grpc::InsecureChannelCredentials()));
    agrpc::GrpcContext grpc_context{std::make_unique<grpc::CompletionQueue>()};

    ConnectedClient client{grpc_context, *stub};
    asio::co_spawn(grpc_context, client.run(), asio::detached);

    std::thread grpc_context_thread{[&]
                                    {
                                        grpc_context.run();
                                    }};

    asio::thread_pool threads{10};
    for (size_t i = 0; i < 10; ++i)
    {
        asio::post(threads,
                   [&]()
                   {
                       for (size_t i = 0; i < 10; ++i)
                       {
                           ole::v1::Request request;
                           request.set_input("Hello " + std::to_string(i));
                           auto data = client.get_additional_data(request);
                           std::osyncstream{std::cout} << "Data: " << data.data()
                                                       << " Thread: " << std::this_thread::get_id() << std::endl;
                           std::this_thread::sleep_for(std::chrono::milliseconds(
                               std::hash<std::thread::id>{}(std::this_thread::get_id()) % 150));
                       }
                   });
    }

    grpc_context_thread.join();
    data_provider_thread.join();
    threads.join();
}
