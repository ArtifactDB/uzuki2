cmake_minimum_required(VERSION 3.24)

project(uzuki2
    VERSION 1.2.0
    DESCRIPTION "Storing simple R lists inside HDF5 or JSON"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

add_library(uzuki2 INTERFACE)
add_library(artifactdb::uzuki2 ALIAS uzuki2)

include(GNUInstallDirs)
target_include_directories(uzuki2 INTERFACE 
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/artifactdb_uzuki2>")

option(UZUKI2_FETCH_EXTERN "Automatically fetch uzuki2's external dependencies." ON)
if(UZUKI2_FETCH_EXTERN)
    add_subdirectory(extern)
else()
    find_package(ltla_byteme CONFIG REQUIRED)
    find_package(ltla_millijson CONFIG REQUIRED)
    find_package(artifactdb_ritsuko CONFIG REQUIRED)
endif()

target_link_libraries(uzuki2 INTERFACE ltla::byteme ltla::millijson artifactdb::ritsuko)

option(UZUKI2_FIND_HDF5 "Try to find and link to HDF5 for uzuki2." ON)
if(UZUKI2_FIND_HDF5)
    find_package(HDF5 COMPONENTS C CXX)
    if (HDF5_FOUND)
        target_link_libraries(uzuki2 INTERFACE hdf5::hdf5 hdf5::hdf5_cpp)
    endif()
endif()

# Building the test-related machinery, if we are compiling this library directly.
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    option(UZUKI2_TESTS "Build uzuki2's test suite." ON)
else()
    option(UZUKI2_TESTS "Build uzuki2's test suite." OFF)
endif()

if(UZUKI2_TESTS)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(tests)
    endif() 
endif()

# Setting up the installation commands.
include(CMakePackageConfigHelpers)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/artifactdb_uzuki2)

install(TARGETS uzuki2 
    EXPORT uzuki2Targets)

install(EXPORT uzuki2Targets
    FILE artifactdb_uzuki2Targets.cmake
    NAMESPACE artifactdb::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/artifactdb_uzuki2)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/artifactdb_uzuki2Config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/artifactdb_uzuki2)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/artifactdb_uzuki2Config.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/artifactdb_uzuki2)

