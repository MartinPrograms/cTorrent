#!/bin/bash

# test.sh

#!/bin/bash
# Build cmake to cmake-build-debug/release
# Usage: ./build.sh [debug|release] (default is debug)

# build using the clang compiler
BUILD_TYPE="debug"

# Enable the BUILD_TESTS option
cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -B "cmake-build-$BUILD_TYPE" -S . -G "Ninja" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBUILD_TESTS=ON

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

# Run tests
cd cmake-build-$BUILD_TYPE || exit 1
cd tests || exit 1
ctest --output-on-failure
if [ $? -ne 0 ]; then
    echo "Tests failed."
    exit 1
fi

echo "All tests passed successfully."