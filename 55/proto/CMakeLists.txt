add_library(zprobe_grpc_proto OBJECT)

asio_grpc_protobuf_generate(
  GENERATE_GRPC
  TARGET
  zprobe_grpc_proto
  USAGE_REQUIREMENT
  PUBLIC
  OUT_DIR
  "${CMAKE_CURRENT_BINARY_DIR}/generated"
  IMPORT_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}"
  PROTOS
  "${CMAKE_CURRENT_SOURCE_DIR}/helloworld/helloworld.proto")

target_compile_features(zprobe_grpc_proto PRIVATE cxx_std_20)

target_link_libraries(zprobe_grpc_proto PRIVATE gRPC::grpc++_unsecure)
