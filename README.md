# Example dispatch bidirectional stream to thread_pool

1. Install [vcpkg](https://github.com/microsoft/vcpkg). At the time of writing this can be achieved by running:

```shell
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat   # or .sh
```

2. Set environment variable `VCPKG_ROOT` to the directory you cloned vcpkg into.

```shell
$env:VCPKG_ROOT="<path-to-vcpkg>"
```

```shell
export VCPKG_ROOT="<path-to-vcpkg>"
```

3. Configure and build with CMake. In the root of this repository run:

```shell
cmake --preset default
cmake --build --preset default
```