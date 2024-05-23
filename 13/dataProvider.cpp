#include "dataProvider.hpp"

#include "ole/v1/dataProvider.grpc.pb.h"

#include <agrpc/asioGrpc.hpp>
#include <boost/asio/bind_executor.hpp>
#include <grpcpp/server_builder.h>

#include <string_view>

void run_data_provider(std::string_view host)
{
    namespace asio = boost::asio;

    grpc::ServerBuilder builder;
    std::unique_ptr<grpc::Server> server;
    ole::v1::DataProvider::AsyncService service;
    agrpc::GrpcContext grpc_context{builder.AddCompletionQueue()};
    builder.AddListeningPort(std::string{host}, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    server = builder.BuildAndStart();

    agrpc::repeatedly_request(&ole::v1::DataProvider::AsyncService::RequestGet, service,
                              asio::bind_executor(grpc_context,
                                                  [&](auto&, auto& reader_writer) -> asio::awaitable<void>
                                                  {
                                                      ole::v1::Request request;
                                                      ole::v1::Data data;
                                                      bool write_ok{true};
                                                      while (write_ok && co_await agrpc::read(reader_writer, request))
                                                      {
                                                          data.set_id(request.id());
                                                          data.set_data(request.input());
                                                          write_ok = co_await agrpc::write(reader_writer, data);
                                                      }
                                                      co_await agrpc::finish(reader_writer, grpc::Status::OK);
                                                  }));

    grpc_context.run();
}