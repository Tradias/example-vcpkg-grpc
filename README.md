# Example protobuf/gRPC project using vcpkg

1. Install [vcpkg](https://github.com/microsoft/vcpkg). At the time of writing this can be achieved by running:

```shell
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat   # or .sh
```

2. Configure CMake. Replace `<vcpkg_root>` with the directory you cloned vcpkg into. In the root of this repository run:

```shell
cmake -B build "-DCMAKE_TOOLCHAIN_FILE=<vcpkg_root>/scripts/buildsystems/vcpkg.cmake"
cmake --build ./build
```

# Branches

This repository has several branches that implement solutions for issues reported to [asio-grpc](https://github.com/Tradias/asio-grpc).

* [issue 13](https://github.com/Tradias/asio-grpc/issues/13): Synchronous interaction with an asynchronous bidirectional stream.
* [issue 14](https://github.com/Tradias/asio-grpc/issues/14): Client with multiple GrpcContexts and grpc::Channels, picked using round-robin.
* [issue 16](https://github.com/Tradias/asio-grpc/issues/16): Read from bidirectional stream and dispatch to thread_pool to compute response.
* [issue 69](https://github.com/Tradias/asio-grpc/issues/69): Long-lived streaming from server to client. (subject/producer/observer pattern)