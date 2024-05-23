#include <agrpc/asio_grpc.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <helloworld/helloworld.grpc.pb.h>

int main(int argc, const char **argv) {
  const auto port = argc >= 2 ? argv[1] : "50051";
  const auto host = std::string("0.0.0.0:") + port;

  std::unique_ptr<grpc::Server> server;

  grpc::ServerBuilder builder;
  agrpc::GrpcContext grpc_context{builder.AddCompletionQueue()};
  builder.AddListeningPort(host, grpc::InsecureServerCredentials());
  helloworld::Greeter::AsyncService service;
  builder.RegisterService(&service);
  server = builder.BuildAndStart();

  asio::co_spawn(
      grpc_context,
      [&]() -> asio::awaitable<void> {
        grpc::ServerContext server_context;
        helloworld::HelloRequest request;
        grpc::ServerAsyncResponseWriter<helloworld::HelloReply> writer{
            &server_context};
        co_await agrpc::request(
            &helloworld::Greeter::AsyncService::RequestSayHello, service,
            server_context, request, writer, asio::use_awaitable);
        helloworld::HelloReply response;
        response.set_message("Hello " + request.name());
        co_await agrpc::finish(writer, response, grpc::Status::OK,
                               asio::use_awaitable);
      },
      asio::detached);

  grpc_context.run();

  server->Shutdown();
}