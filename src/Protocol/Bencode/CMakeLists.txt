FILE(GLOB_RECURSE
    ben_code_c_sources
    *.c
)

FILE(GLOB_RECURSE
    ben_code_h_sources
    *.h
)

add_library(ben_code STATIC ${ben_code_c_sources})

# This library relies on the core_file library, so we need to link it
target_link_libraries(ben_code
    PUBLIC
        core_file
        core_string
        core_generic
)

target_include_directories(ben_code
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)