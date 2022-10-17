cmake_minimum_required(VERSION 3.14)

project(uzuki2
    VERSION 1.0.0
    DESCRIPTION "Storing simple R lists inside HDF5"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

add_library(uzuki2 INTERFACE)

target_include_directories(uzuki2 INTERFACE include)

add_subdirectory(extern)

target_link_libraries(uzuki2 INTERFACE byteme)

# Building the test-related machinery, if we are compiling this library directly.
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    option(INSTALL_GTEST OFF)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(tests)
    endif() 
endif()

