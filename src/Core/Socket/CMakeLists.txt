FILE(GLOB_RECURSE
        core_socket_c_sources
        *.c
)

FILE(GLOB_RECURSE
        core_socket_h_sources
        *.h
)

add_library(core_socket STATIC ${core_socket_c_sources})
target_include_directories(core_socket
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)