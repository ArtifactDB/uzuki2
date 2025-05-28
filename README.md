# R lists to HDF5 or JSON

![Unit tests](https://github.com/ArtifactDB/uzuki2/actions/workflows/run-tests.yaml/badge.svg)
![Documentation](https://github.com/ArtifactDB/uzuki2/actions/workflows/doxygenate.yaml/badge.svg)
[![codecov](https://codecov.io/gh/ArtifactDB/uzuki2/branch/master/graph/badge.svg?token=J3dxS3MtT1)](https://codecov.io/gh/ArtifactDB/uzuki2)

## Overview

The **uzuki2** repository describes a language-agnostic file format for serializing basic [R](https://r-project.org) lists.
List elements may be atomic vectors, `NULL`, or nested lists of such objects.
It also supports missing values in the vectors and per-element names on the vectors or lists.
A mechanism is also provided to handle external references to more complex objects (e.g., S4 classes) that cannot be directly saved into the format.

## Specifications

We support serialization in either HDF5 or (possibly Gzip-compressed) JSON.
Both of these are widely used formats and have complementary strengths for list representation.
HDF5 supports random access into list components, which can provide optimization opportunities when the list is large and/or contains large atomic vectors.
In contrast, JSON is easier to parse and has less storage overhead per list element.

Both the HDF5 and JSON specifications have multiple versions. 
Links to the version-specific HDF5 specifications are listed below, along with the minimum version of the C++ library required to parse them:

- [1.4](https://github.com/ArtifactDB/uzuki2/tree/gh-pages/docs/specifications/hdf5-1.4.md), supported by **uzuki2** version ≥ 1.5.
- [1.3](https://github.com/ArtifactDB/uzuki2/tree/gh-pages/docs/specifications/hdf5-1.3.md), supported by **uzuki2** version ≥ 1.3.
- [1.2](https://github.com/ArtifactDB/uzuki2/tree/gh-pages/docs/specifications/hdf5-1.2.md), supported by **uzuki2** version ≥ 1.2.
- [1.1](https://github.com/ArtifactDB/uzuki2/tree/gh-pages/docs/specifications/hdf5-1.1.md), supported by **uzuki2** version ≥ 1.1.
- [1.0](https://github.com/ArtifactDB/uzuki2/tree/gh-pages/docs/specifications/hdf5-1.0.md), supported by **uzuki2** version ≥ 1.0.

Similarly, different versions of the JSON specification are listed below:

- [1.2](https://github.com/ArtifactDB/uzuki2/tree/gh-pages/docs/specifications/json-1.2.md), supported by **uzuki2** version ≥ 1.2.
- [1.1](https://github.com/ArtifactDB/uzuki2/tree/gh-pages/docs/specifications/json-1.1.md), supported by **uzuki2** version ≥ 1.1.
- [1.0](https://github.com/ArtifactDB/uzuki2/tree/gh-pages/docs/specifications/json-1.0.md), supported by **uzuki2** version ≥ 1.0.

## Validation

### Quick start

A reference implementation of the validator is provided as a header-only C++ library in [`include/uzuki2`](include/uzuki2).
This is useful for portable deployment in different frameworks like R, Python, etc.
We can check that a JSON/HDF5 file complies with the **uzuki** specification:

```cpp
#include "uzuki2/uzuki2.hpp"
uzuki2::hdf5::validate(h5_file_path, h5_group_name, 0, {});
uzuki2::json::validate(json_file_path, 0, {});
```

This will raise an error if any violations of the specification are observed.
If a non-zero expected number of external objects is present:

```cpp
uzuki2::hdf5::validate(h5_file_path, h5_group_name, num_externals, {});
```

Each function accepts a `struct` of additional options to fine-tune its behavior:

```cpp
uzuki2::json::validate(json_file_path, 0, [&]{
    uzuki2::json::Options opt;
    opt.strict_list = false;
    opt.buffer_size = 1048576;
    opt.parallel = true;
    return opt;
}());
```

Advanced users can also use the **uzuki2** parser to load the list into memory.
This is achieved by calling `parse()` with custom provisioner and external reference classes.
For example, [`tests/src/test_subclass.h`](tests/src/test_subclass.h) defines the `DefaultProvisioner` and `DefaultExternals` classes,
which can be used to load the HDF5 contents into `std::vector`s for easier downstream operations.

```cpp
DefaultExternals ext(nexpected);
auto ptr = uzuki2::hdf5::parse<DefaultProvisioner>(file_path, group_name, ext, {});
```

See the [reference documentation](https://artifactdb.github.io/uzuki2) for more details.

### Building projects

#### CMake with `FetchContent`

If you're using CMake, you just need to add something like this to your `CMakeLists.txt`:

```cmake
include(FetchContent)

FetchContent_Declare(
  uzuki2 
  GIT_REPOSITORY https://github.com/ArtifactDB/uzuki2
  GIT_TAG master # or any version of interest
)

FetchContent_MakeAvailable(uzuki2)
```

Then you can link to **uzuki2** to make the headers available during compilation:

```cmake
# For executables:
target_link_libraries(myexe uzuki2)

# For libaries
target_link_libraries(mylib INTERFACE uzuki2)
```

By default, this will use `FetchContent` to fetch all external dependencies.
Applications are advised to pin the versions of all dependencies themselves - see [`extern/CMakeLists.txt`](extern/CMakeLists.txt) for suggested versions.
If you want to install them manually, use `-DUZUKI2_FETCH_EXTERN=OFF`.

#### CMake with `find_package()`

You can install the library by cloning a suitable version of this repository and running the following commands:

```sh
mkdir build && cd build
cmake .. -DUZUKI2_TESTS=OFF
cmake --build . --target install
```

Then you can use `find_package()` as usual:

```cmake
find_package(artifactdb_uzuki2 CONFIG REQUIRED)
target_link_libraries(mylib INTERFACE artifactdb::uzuki2)
```

Again, this will use `FetchContent` to fetch dependencies, see comments above.

#### Manual

If you're not using CMake, the simple approach is to just copy the files in the `include/` subdirectory - 
either directly or with Git submodules - and include their path during compilation with, e.g., GCC's `-I`.
You will also need to link to the dependencies listed in the [`extern/CMakeLists.txt`](extern/CMakeLists.txt) directory,
along with the HDF5 and Zlib libraries.

## Further comments

See [here](docs/specifications/misc.md#comparison-to-version-1) for a list of changes from the original [**uzuki**](https://github.com/LTLA/uzuki) library.

Just like the original **uzuki**, we're just re-using the reference to [Uzuki Shimamura](https://myanimelist.net/character/70883/Uzuki_Shimamura) for the name:

![Uzuki Shimamura](https://media1.giphy.com/media/7Oy2FDqWV5mak/giphy.gif)
