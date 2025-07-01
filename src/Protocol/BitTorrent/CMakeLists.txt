FILE(GLOB_RECURSE
        protocol_bittorrent_c_sources
        *.c
)

FILE(GLOB_RECURSE
        protocol_bittorrent_h_sources
        *.h
)

add_library(protocol_bittorrent STATIC ${protocol_bittorrent_c_sources})
target_link_libraries(protocol_bittorrent
        PUBLIC
        core_generic
        core_file
        core_socket
        ben_code
        core_networking
)
target_include_directories(protocol_bittorrent
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)