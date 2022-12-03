# Helloworld asio-grpc server using standalone Asio

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