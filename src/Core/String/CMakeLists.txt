FILE(GLOB_RECURSE
    core_string_c_sources
    *.c
)

FILE(GLOB_RECURSE
    core_string_h_sources
    *.h
)

add_library(core_string STATIC ${core_string_c_sources})
target_include_directories(core_string
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)