cmake_minimum_required(VERSION 3.30)
project(cTorrent VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/release)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
            "Choose the type of build (Debug or Release)" FORCE)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -Werror")

# Disable Wunused-* (they're annoying, and barely useful)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function")

# Include the src/Core/File directory
add_subdirectory(src/Core/File)
add_subdirectory(src/Core/String)

add_subdirectory(src/Protocol/Bencode)

# The CLI application
add_executable(cTorrent
    src/Application/CLI/main.c
)

install(TARGETS cTorrent RUNTIME DESTINATION bin)
target_link_libraries(cTorrent PRIVATE core_file)
target_include_directories(cTorrent PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

if (BUILD_TESTS)
    # if we're testing add the -Wno-gnu-zero-variadic-macro-arguments
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-gnu-zero-variadic-macro-arguments")
    add_subdirectory(tests)
endif()

# Install the headers
install(DIRECTORY src
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)