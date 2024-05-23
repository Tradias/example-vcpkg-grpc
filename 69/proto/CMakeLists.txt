add_library(rogerworld_grpc_proto OBJECT)

asio_grpc_protobuf_generate(
  GENERATE_GRPC
  TARGET
  rogerworld_grpc_proto
  USAGE_REQUIREMENT
  PUBLIC
  OUT_DIR
  "${CMAKE_CURRENT_BINARY_DIR}/generated"
  IMPORT_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}"
  PROTOS
  "${CMAKE_CURRENT_SOURCE_DIR}/rogerworld/rogerworld.proto")

target_compile_features(rogerworld_grpc_proto PRIVATE cxx_std_20)

target_link_libraries(rogerworld_grpc_proto PRIVATE gRPC::grpc++_unsecure)
