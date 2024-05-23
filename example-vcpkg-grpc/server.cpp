#include "helloworld.grpc.pb.h"

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <thread>

class Service : public helloworld::Greeter::Service {
  grpc::Status SayHello(grpc::ServerContext *context,
                        const helloworld::HelloRequest *request,
                        helloworld::HelloReply *response) override {
    response->set_message("Hello " + request->name());
    return grpc::Status::OK;
  }
};

int main() {
  grpc::ServerBuilder builder;
  std::unique_ptr<grpc::Server> server;
  Service service;
  builder.AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  server = builder.BuildAndStart();
  std::this_thread::sleep_for(std::chrono::seconds(10));
  server->Shutdown();
}