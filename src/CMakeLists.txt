add_executable(example)

target_sources(example PRIVATE main.cpp)

target_link_libraries(example PRIVATE asio-grpc::asio-grpc example_grpc_proto)

target_compile_features(example PRIVATE cxx_std_20)

target_compile_options(example PUBLIC $<$<CXX_COMPILER_ID:GNU>:-fcoroutines>)
