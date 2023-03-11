add_executable(rogerworld)

target_sources(rogerworld PRIVATE main.cpp)

target_link_libraries(rogerworld PRIVATE asio-grpc::asio-grpc-standalone-asio rogerworld_grpc_proto)

target_compile_features(rogerworld PRIVATE cxx_std_20)

target_compile_options(rogerworld PUBLIC $<$<CXX_COMPILER_ID:GNU>:-fcoroutines>)
