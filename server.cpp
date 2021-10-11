#include "protos/example.grpc.pb.h"

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

int main() {
  grpc::ServerBuilder builder;
  std::unique_ptr<grpc::Server> server;
  example::v1::Example::AsyncService service;
  auto cq = builder.AddCompletionQueue();
  builder.AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  server = builder.BuildAndStart();

  grpc::ServerContext server_context;
  example::v1::Request request;
  grpc::ServerAsyncResponseWriter<example::v1::Response> writer{
      &server_context};

  service.RequestUnary(&server_context, &request, &writer, cq.get(), cq.get(),
                       nullptr);

  server->Wait();
}