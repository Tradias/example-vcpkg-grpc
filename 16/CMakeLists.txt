cmake_minimum_required(VERSION 3.15)

project(test)

find_package(asio-grpc CONFIG REQUIRED)

add_library(grpc-sources OBJECT)
asio_grpc_protobuf_generate(
    GENERATE_GRPC
    TARGET grpc-sources
    USAGE_REQUIREMENT PUBLIC
    OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated"
    IMPORT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/proto"
    PROTOS "${CMAKE_CURRENT_SOURCE_DIR}/proto/muusbolla/muusbolla.proto")
target_compile_definitions(
    grpc-sources
    PUBLIC BOOST_ASIO_NO_DEPRECATED
           $<$<CXX_COMPILER_ID:MSVC>:
           BOOST_ASIO_HAS_DEDUCED_REQUIRE_MEMBER_TRAIT
           BOOST_ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT
           BOOST_ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT
           BOOST_ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT
           BOOST_ASIO_HAS_DEDUCED_PREFER_MEMBER_TRAIT>)
target_compile_features(grpc-sources PUBLIC cxx_std_20)
target_link_libraries(grpc-sources PRIVATE asio-grpc::asio-grpc)
target_precompile_headers(grpc-sources PRIVATE "precompiled_header.hpp")

add_executable(client)
target_sources(client PRIVATE client.cpp)
target_link_libraries(client PRIVATE asio-grpc::asio-grpc grpc-sources)
target_compile_options(client PRIVATE $<$<CXX_COMPILER_ID:GNU>:-fcoroutines>)
target_include_directories(client PRIVATE "${CMAKE_CURRENT_LIST_DIR}")
target_precompile_headers(client REUSE_FROM grpc-sources)

add_executable(server)
target_sources(server PRIVATE server.cpp)
target_link_libraries(server PRIVATE asio-grpc::asio-grpc grpc-sources)
target_compile_options(server PRIVATE $<$<CXX_COMPILER_ID:GNU>:-fcoroutines>)
target_include_directories(server PRIVATE "${CMAKE_CURRENT_LIST_DIR}")
target_precompile_headers(server REUSE_FROM grpc-sources)
