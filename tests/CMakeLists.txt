include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/c9461a9b55ba954df0489bab6420eb297bed846b.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Main test executable.
add_executable(
    libtest 
    src/list.cpp
    src/integer.cpp
    src/boolean.cpp
    src/factor.cpp
    src/number.cpp
    src/string.cpp
    src/date.cpp
    src/external.cpp
    src/misc.cpp
)

target_link_libraries(
    libtest
    gtest_main
    gmock_main 
    uzuki2
)

find_package(HDF5 REQUIRED COMPONENTS C CXX)
target_link_libraries(libtest hdf5::hdf5 hdf5::hdf5_cpp)

set(CODE_COVERAGE OFF CACHE BOOL "Enable coverage testing")
if(CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(libtest PRIVATE -O0 -g --coverage)
    target_link_options(libtest PRIVATE --coverage)
endif()

include(GoogleTest)
gtest_discover_tests(libtest)
