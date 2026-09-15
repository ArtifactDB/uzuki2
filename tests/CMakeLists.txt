include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/7da55820cc32dedd6c1b048f2d4e13fdde5e8237.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Avoid installing GoogleTest when installing this project.
option(INSTALL_GTEST "Enable installation of googletest." OFF)

FetchContent_MakeAvailable(googletest)

enable_testing()

# Main test executable.
add_executable(
    libtest 

    src/hdf5/list.cpp
    src/hdf5/integer.cpp
    src/hdf5/boolean.cpp
    src/hdf5/factor.cpp
    src/hdf5/number.cpp
    src/hdf5/string.cpp
    src/hdf5/vls.cpp
    src/hdf5/names.cpp
    src/hdf5/date.cpp
    src/hdf5/datetime.cpp
    src/hdf5/format.cpp
    src/hdf5/external.cpp
    src/hdf5/general.cpp

    src/json/list.cpp
    src/json/integer.cpp
    src/json/boolean.cpp
    src/json/factor.cpp
    src/json/number.cpp
    src/json/string.cpp
    src/json/names.cpp
    src/json/date.cpp
    src/json/datetime.cpp
    src/json/format.cpp
    src/json/external.cpp
    src/json/general.cpp
)

target_link_libraries(
    libtest
    gtest_main
    gmock_main 
    uzuki2
)

target_compile_options(libtest PRIVATE -Wall -Wextra -Wpedantic -Werror)

set(CODE_COVERAGE OFF CACHE BOOL "Enable coverage testing")
if(CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(libtest PRIVATE -O0 -g --coverage)
    target_link_options(libtest PRIVATE --coverage)
endif()

include(GoogleTest)
gtest_discover_tests(libtest)
