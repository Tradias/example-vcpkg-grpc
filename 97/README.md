# GRPC helloworld server

1. Install [vcpkg](https://github.com/microsoft/vcpkg). At the time of writing this can be achieved by running:

```shell
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat   # or .sh
```

2. Configure CMake. Make sure env var `VCPKG_ROOT` points at the directory you cloned vcpkg into. In the root of this repository run:

```shell
cmake --preset default
cmake --build --preset default
```