FILE(GLOB_RECURSE
    core_file_c_sources
    *.c
)

FILE(GLOB_RECURSE
    core_file_h_sources
    *.h
)

add_library(core_file STATIC ${core_file_c_sources})
target_include_directories(core_file
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)