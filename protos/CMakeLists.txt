set(PROTO_FILES helloworld.proto)

# library target with protobuf sources
add_library(protobuf-objects OBJECT)

target_sources(protobuf-objects PRIVATE ${PROTO_FILES})

target_link_libraries(protobuf-objects PUBLIC gRPC::grpc++)

target_include_directories(protobuf-objects PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate(TARGET protobuf-objects LANGUAGE cpp)

protobuf_generate(
  TARGET
  protobuf-objects
  LANGUAGE
  grpc
  GENERATE_EXTENSIONS
  .grpc.pb.h
  .grpc.pb.cc
  PLUGIN
  "protoc-gen-grpc=\$<TARGET_FILE:gRPC::grpc_cpp_plugin>")
