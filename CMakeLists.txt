cmake_minimum_required(VERSION 3.10.0)

project(draconity)
set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
SET(CMAKE_SKIP_BUILD_RPATH TRUE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
include_directories(src)
if (DEBUG)
    add_definitions(-O0 -ggdb -fsanitize=address -fno-omit-frame-pointer)
    link_directories(/Library/Developer/CommandLineTools/usr/lib/clang/9.0.0/lib/darwin/)
else()
    add_definitions(-O2)
endif()

file(GLOB_RECURSE SOURCE src/*.c)
add_library(draconity SHARED ${SOURCE})
target_link_libraries(draconity m dl pthread czmq bson-1.0)
if (DEBUG)
    target_link_libraries(draconity clang_rt.asan_osx_dynamic)
endif()

add_custom_command(TARGET draconity
    POST_BUILD COMMAND
    ${CMAKE_INSTALL_NAME_TOOL} -add_rpath "@executable_path"
    $<TARGET_FILE:draconity>)
