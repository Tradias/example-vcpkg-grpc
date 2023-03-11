#include <agrpc/asio_grpc.hpp>
#include <asio/as_tuple.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <asio/experimental/channel.hpp>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <rogerworld/rogerworld.grpc.pb.h>

#include <list>

using ResponseSPtr = std::shared_ptr<rogerworld::Response>;

struct Subject
{
    using Channel = asio::experimental::channel<void(std::error_code, ResponseSPtr)>;

    std::list<Channel> channels;
};

asio::awaitable<void> for_each_channel(Subject& subject, auto action)
{
    for (auto it = subject.channels.begin(); it != subject.channels.end();)
    {
        if (it->is_open())
        {
            co_await action(*it);
            ++it;
        }
        else
        {
            it = subject.channels.erase(it);
        }
    }
}

asio::awaitable<void> notify_all(Subject& subject, ResponseSPtr response)
{
    co_await for_each_channel(subject,
                              [&](Subject::Channel& channel)
                              {
                                  return channel.async_send(std::error_code{}, response, asio::use_awaitable);
                              });
}

asio::awaitable<void> close(Subject& subject)
{
    co_await for_each_channel(subject,
                              [&](Subject::Channel& channel)
                              {
                                  return channel.async_send(std::error_code{asio::error::operation_aborted}, nullptr,
                                                            asio::use_awaitable);
                              });
}

auto& add_observer(Subject& subject, auto executor) { return subject.channels.emplace_back(executor); }

void remove_observer(Subject::Channel& observer) { observer.close(); }

asio::awaitable<void> produce_data(agrpc::GrpcContext& grpc_context, Subject& subject)
{
    for (int i{}; i < 5; ++i)
    {
        auto data = std::make_shared<rogerworld::Response>();

        // Fill data. Emulating some time passing:
        co_await agrpc::Alarm(grpc_context).wait(std::chrono::system_clock::now() + std::chrono::seconds(2));
        data->set_data(i);

        co_await notify_all(subject, std::move(data));
    }
    co_await close(subject);
}

asio::awaitable<void> handle_request(Subject& subject, grpc::ServerAsyncWriter<rogerworld::Response>& responder)
{
    bool ok{true};
    auto& observer = add_observer(subject, co_await asio::this_coro::executor);
    while (ok)
    {
        const auto [ec, response] = co_await observer.async_receive(asio::as_tuple(asio::use_awaitable));
        if (ec)
        {
            break;
        }
        ok = co_await agrpc::write(responder, *response);
    }
    remove_observer(observer);
    co_await agrpc::finish(responder, grpc::Status::OK);
}

int main(int argc, const char** argv)
{
    const auto port = argc >= 2 ? argv[1] : "50051";
    const auto host = std::string("0.0.0.0:") + port;

    std::unique_ptr<grpc::Server> server;

    grpc::ServerBuilder builder;
    agrpc::GrpcContext grpc_context{builder.AddCompletionQueue()};
    builder.AddListeningPort(host, grpc::InsecureServerCredentials());
    rogerworld::Longlived::AsyncService service;
    builder.RegisterService(&service);
    server = builder.BuildAndStart();

    Subject subject;
    bool producer_started{};
    agrpc::repeatedly_request(
        &rogerworld::Longlived::AsyncService::RequestSubscribe, service,
        asio::bind_executor(grpc_context,
                            [&](auto&, auto&, auto& responder) -> asio::awaitable<void>
                            {
                                // I cheat a bit by having only one producer. You might want to
                                // create some kind of list of producers and start one only if not
                                // already in the list
                                if (!std::exchange(producer_started, true))
                                {
                                    asio::co_spawn(grpc_context, produce_data(grpc_context, subject), asio::detached);
                                }
                                co_await handle_request(subject, responder);
                            }));

    grpc_context.run();

    server->Shutdown();
}