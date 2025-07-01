enable_testing()

FILE(GLOB_RECURSE
    test_c_sources
    *.c
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CHECK REQUIRED check)

include_directories(${CHECK_INCLUDE_DIRS})
link_directories(${CHECK_LIBRARY_DIRS})

foreach(test_source ${test_c_sources})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source})

    target_link_libraries(${test_name} PRIVATE ${CHECK_LIBRARIES} core_generic core_file core_string core_socket ben_code)
    target_include_directories(${test_name} PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
    )

    message("Test name: ${test_name}")
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach ()

