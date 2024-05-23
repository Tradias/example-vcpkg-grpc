cmake_minimum_required(VERSION 3.15..3.25)

project(example)

find_package(asio-grpc CONFIG REQUIRED)

add_subdirectory(proto)

add_subdirectory(src)
