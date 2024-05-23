This repository contains a:

[Simple project that uses vcpkg to get gRPC and generate proto sources during build](./example-vcpkg-grpc/README)

And several solutions for issues reported to [asio-grpc](https://github.com/Tradias/asio-grpc):

* [issue 13](https://github.com/Tradias/asio-grpc/issues/13): Synchronous interaction with an asynchronous bidirectional stream
* [issue 14](https://github.com/Tradias/asio-grpc/issues/14): Client with multiple GrpcContexts and grpc::Channels, picked using round-robin
* [issue 16](https://github.com/Tradias/asio-grpc/issues/16): Read from bidirectional stream and dispatch to thread_pool to compute response
* [issue 55](https://github.com/Tradias/asio-grpc/issues/55): Helloworld server using standalone Asio
* [issue 69](https://github.com/Tradias/asio-grpc/issues/69): Long-lived streaming from server to client (subject/producer/observer pattern)
* [issue 81](https://github.com/Tradias/asio-grpc/issues/81): Long-lived gRPC server stream
* [issue 97](https://github.com/Tradias/asio-grpc/issues/97): Helloworld server using Boost.Asio