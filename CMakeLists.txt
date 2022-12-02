cmake_minimum_required(VERSION 3.15)

project(zprobe)

find_package(asio-grpc CONFIG REQUIRED)

add_subdirectory(proto)

add_subdirectory(src)
