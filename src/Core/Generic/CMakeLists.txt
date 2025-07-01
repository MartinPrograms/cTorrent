FILE(GLOB_RECURSE
        core_generic_c_sources
        *.c
)

FILE(GLOB_RECURSE
        core_generic_h_sources
        *.h
)

add_library(core_generic STATIC ${core_generic_c_sources})
target_include_directories(core_generic
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)