cmake_minimum_required(VERSION 3.15)

project(test)

find_package(asio-grpc CONFIG REQUIRED)

add_executable(test main.cpp)

target_sources(test PRIVATE main.cpp dataProvider.cpp)

target_link_libraries(test PRIVATE asio-grpc::asio-grpc)

asio_grpc_protobuf_generate(
    GENERATE_GRPC
    TARGET test
    OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated"
    IMPORT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/proto"
    PROTOS "${CMAKE_CURRENT_SOURCE_DIR}/proto/ole/v1/dataProvider.proto")

target_include_directories(test PRIVATE "${CMAKE_CURRENT_LIST_DIR}")

target_compile_definitions(
    test
    PRIVATE BOOST_ASIO_NO_DEPRECATED
            $<$<CXX_COMPILER_ID:MSVC>:
            BOOST_ASIO_HAS_DEDUCED_REQUIRE_MEMBER_TRAIT
            BOOST_ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT
            BOOST_ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT
            BOOST_ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT
            BOOST_ASIO_HAS_DEDUCED_PREFER_MEMBER_TRAIT>)

target_compile_features(test PRIVATE cxx_std_20)

target_precompile_headers(test PRIVATE "precompiledHeader.h")
