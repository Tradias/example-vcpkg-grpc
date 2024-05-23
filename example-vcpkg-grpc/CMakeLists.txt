cmake_minimum_required(VERSION 3.19)

project(
  example-vcpkg-grpc
  VERSION 0.1.0
  DESCRIPTION "Example protobuf/gRPC project using vcpkg"
  HOMEPAGE_URL "https://github.com/Tradias/example-vcpkg-grpc"
  LANGUAGES CXX)

find_package(gRPC REQUIRED)

add_subdirectory(protos)

# main
add_executable(server)

target_sources(server PRIVATE server.cpp)

target_link_libraries(server PRIVATE protobuf-objects gRPC::grpc++)
