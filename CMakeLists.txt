cmake_minimum_required(VERSION 3.19)

project(
  example-vcpkg-grpc
  VERSION 0.1.0
  DESCRIPTION "Example protobuf/gRPC project using vcpkg"
  HOMEPAGE_URL "https://github.com/Tradias/example-vcpkg-grpc"
  LANGUAGES CXX)

find_package(gRPC REQUIRED)

# An example using
# https://cmake.org/cmake/help/latest/module/FindProtobuf.html#command:protobuf_generate_cpp
protobuf_generate_cpp(HELLOWORLD_SOURCES HELLOWORLD_HEADERS
                      "${CMAKE_CURRENT_LIST_DIR}/protos/helloworld.proto")

add_executable(example-protobuf)

target_sources(example-protobuf PRIVATE protobuf.cpp ${HELLOWORLD_SOURCES}
                                        ${HELLOWORLD_HEADERS})

target_link_libraries(example-protobuf PRIVATE protobuf::libprotobuf)

target_include_directories(example-protobuf PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# An example using
# https://github.com/Tradias/asio-grpc/blob/master/cmake/AsioGrpcProtobufGenerator.cmake
include("${CMAKE_CURRENT_LIST_DIR}/AsioGrpcProtobufGenerator.cmake")

set(EXAMPLE_PROTOS_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
asio_grpc_protobuf_generate(
  GENERATE_GRPC
  OUT_VAR
  EXAMPLE_SOURCES
  PROTOC_OUT_DIR
  "${EXAMPLE_PROTOS_OUT_DIR}"
  PROTOS
  "${CMAKE_CURRENT_LIST_DIR}/protos/example.proto")

add_executable(example-grpc)

target_sources(example-grpc PRIVATE server.cpp ${EXAMPLE_SOURCES})

target_link_libraries(example-grpc PRIVATE gRPC::grpc++)

target_include_directories(example-grpc PRIVATE ${EXAMPLE_PROTOS_OUT_DIR})
