#!/bin/bash
# Build cmake to cmake-build-debug/release
# Usage: ./build.sh [debug|release] (default is debug)

# build using the clang compiler

if [ -z "$1" ]; then
    BUILD_TYPE="debug"
else
    BUILD_TYPE="$1"
fi

if [ "$BUILD_TYPE" != "debug" ] && [ "$BUILD_TYPE" != "release" ]; then
    echo "Invalid argument. Use 'debug' or 'release'."
    exit 1
fi

if [ ! -d "cmake-build-$BUILD_TYPE" ]; then
    mkdir "cmake-build-$BUILD_TYPE"
fi

cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -B "cmake-build-$BUILD_TYPE" -S . -G "Ninja" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

if [ $? -ne 0 ]; then
    echo "CMake configuration failed."
    exit 1
fi

cmake --build "cmake-build-$BUILD_TYPE" --config "$BUILD_TYPE"
if [ $? -ne 0 ]; then
    echo "Build failed."
    exit 1
fi

echo "Build completed successfully."
