# Windows
## Debug
cmake --preset windows-debug-vcpkg
cmake --build --preset windows-debug-vcpkg --config Debug

## Release
cmake --preset windows-release-vcpkg
cmake --build --preset windows-release-vcpkg --config Release


# Linux
## Debug (system)
cmake --preset linux-debug
cmake --build --preset linux-debug

## Release (system)
cmake --preset linux-release
cmake --build --preset linux-release

