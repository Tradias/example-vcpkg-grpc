cmake_minimum_required(VERSION 3.15)

project(test)

find_package(asio-grpc CONFIG REQUIRED)

add_executable(main)
asio_grpc_protobuf_generate(
    GENERATE_GRPC
    TARGET main
    USAGE_REQUIREMENT PRIVATE
    OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated"
    IMPORT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/proto"
    PROTOS "${CMAKE_CURRENT_SOURCE_DIR}/proto/helloworld/helloworld.proto")
target_sources(main PRIVATE main.cpp)
target_compile_features(main PRIVATE cxx_std_20)
target_link_libraries(main PRIVATE asio-grpc::asio-grpc)
target_compile_options(main PRIVATE $<$<CXX_COMPILER_ID:GNU>:-fcoroutines>)
target_include_directories(main PRIVATE "${CMAKE_CURRENT_LIST_DIR}")
target_compile_definitions(main INTERFACE BOOST_ASIO_NO_DEPRECATED)
target_precompile_headers(main PRIVATE "precompiled_header.hpp")
