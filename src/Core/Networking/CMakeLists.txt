FILE(GLOB_RECURSE
        core_networking_c_sources
        *.c
)

FILE(GLOB_RECURSE
        core_networking_h_sources
        *.h
)

add_library(core_networking STATIC ${core_networking_c_sources})
# Add the core_file to this library
target_link_libraries(core_networking
        PUBLIC
        core_socket
        core_generic
        core_file
)
target_include_directories(core_networking
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)