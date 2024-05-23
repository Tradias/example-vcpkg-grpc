add_executable(zprobe)

target_sources(zprobe PRIVATE main.cpp)

target_link_libraries(zprobe PRIVATE asio-grpc::asio-grpc-standalone-asio zprobe_grpc_proto)

target_compile_features(zprobe PRIVATE cxx_std_20)

target_compile_options(zprobe PUBLIC $<$<CXX_COMPILER_ID:GNU>:-fcoroutines>)
