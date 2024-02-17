add_executable(example)

target_sources(example PRIVATE main.cpp)

target_link_libraries(example PRIVATE asio-grpc::asio-grpc Boost::headers
                                      example_grpc_proto)

target_include_directories(example PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_compile_features(example PRIVATE cxx_std_20)
