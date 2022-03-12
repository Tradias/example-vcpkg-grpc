#include "helloWorld/helloWorld.grpc.pb.h"

#include <agrpc/asioGrpc.hpp>
#include <boost/asio/bind_executor.hpp>
#include <grpcpp/server_builder.h>

#include <thread>
#include <vector>

int main(int argc, const char** argv)
{
    const auto thread_count = argc >= 2 ? std::stoi(argv[1]) : 1;
    std::string host{"localhost:50051"};

    namespace asio = boost::asio;

    grpc::ServerBuilder builder;
    std::unique_ptr<grpc::Server> server;
    helloworld::Greeter::AsyncService service;
    builder.AddListeningPort(std::string{host}, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::vector<std::unique_ptr<grpc::ServerCompletionQueue>> queues;
    for (size_t i = 0; i < thread_count; ++i)
    {
        queues.emplace_back(builder.AddCompletionQueue());
    }
    server = builder.BuildAndStart();

    std::vector<std::jthread> threads;
    for (size_t i = 0; i < thread_count; ++i)
    {
        threads.emplace_back(
            [&, i]
            {
                agrpc::GrpcContext grpc_context{std::move(queues[i])};
                agrpc::repeatedly_request(
                    &helloworld::Greeter::AsyncService::RequestSayHello, service,
                    asio::bind_executor(grpc_context,
                                        [&](auto&& context)
                                        {
                                            helloworld::HelloReply response;
                                            response.set_message("Hello " + context.request().name());
                                            auto& writer = context.responder();
                                            agrpc::finish(
                                                writer, response, grpc::Status::OK,
                                                asio::bind_executor(grpc_context, [c = std::move(context)](bool) {}));
                                        }));
                grpc_context.run();
            });
    }
}
