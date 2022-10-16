# R lists to HDF5

## Overview

The **uzuki2** repository describes a format for safely serializing basic R lists into HDF5.
List elements may be atomic vectors, `NULL`, or nested lists of such elements.
It also supports factors, dates, missing strings, and names on the vectors or lists.
Some mechanism is also provided to handle external references to non-serializable objects.

In the specification below, we use `**/` to represent a variable name of the group representing any of the supported R objects.
It is assumed that `**/` will be replaced by actual names in implementations,
as defined by users (for the top-level group) or by the specification (e.g., as a nested child of a list).

## Specification

### Lists

An R list is represented as a HDF5 group (`**/`) with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"list"`.

This group should contain a subgroup `**/data` that contains the list elements.
Each list element is itself represented by a subgroup that is named after its 0-based position in the list, e.g., `**/data/0` for the first list element.
Each list element's group may be any of the objects described in this specifiction, including further nested lists.

If the list is named, there will additionally be a 1-dimensional `**/names` string dataset of length equal to `uzuki_length`.

### Atomic vectors

An atomic vector is represented as a HDF5 group (`**/`) with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"atomic"`.
- `uzuki_type`, a scalar string dataset containing one of `"integer"`, `"boolean"`, `"number"`, `"string"`, `"date"`, `"factor"` or `"ordered"`.

The group should contain an 1-dimensional dataset at `**/data`.
The allowed HDF5 datatype depends on `uzuki_type`:

- `"integer"`, `"boolean"`, `"factor"` and `"ordered"`: any type of `H5T_INTEGER` that can be represented by a 32-bit signed integer.
- `"number"`: any type of `H5T_FLOAT` that can be represented by a double-precision float.
- `"string"` or `"date"`: any type of `H5T_STRING` that can be represented by a UTF-8 encoded string.

For some `uzuki_type`, further considerations may be applicable:

- `"integer"`: values of `**/data` that are equal to -2147483648 should be treated as missing.
- `"boolean"`: values in `**/data` should be one of 0 (false), 1 (true), or -2147483648 (missing).
- `"factor"` or `"ordered"`: the atomic vector's group should also contain `**/levels`.
  This is a 1-dimensional string dataset that contains the levels for the indices in `data`.
  Values in `**/levels` should be unique.
  Values in `**/data` should be non-negative and less than the length of `**/levels`;
  except for missing values, which are represented by -2147483648.
- `string`: the `**/data` dataset may contain a `"missing-value-placeholder"` attribute.
  If present, this should be a string scalar dataset that specifies the placeholder for missing values.
  Any value of `**/data` that is equal to this placeholder should be treated as missing.
- `"date"`: like `"string"`, the `**/data` dataset may contain a `missing-value-placeholder` attribute.
  The `**/data` dataset should only contain `YYYY-MM-DD` dates or the placeholder value.

The atomic vector's group may also contain `**/names`, a 1-dimensional string dataset of length equal to `data`.

### Null

A null value is represented as a HDF5 group with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"null"`.

### External object

Each external object is represented as a HDF5 group (`**/`) with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"external"`.

This should contain an `**/index` scalar dataset, containing an index that identifies this external object uniquely within the entire list.
`**/index` should start at zero and be incremented whenever an external object is encountered. 

By indexing this external metadata, we can restore the object in its appropriate location in the list.
The exact mechanism by which this restoration occurs is implementation-defined.

## Validation

### Quick start

A reference implementation of the validator is provided as a header-only C++ library in [`include/uzuki2`](include/uzuki2).
This is useful for portable deployment in different frameworks like R, Python, etc.
We can check that a HDF5 file complies with the **uzuki** specification:

```cpp
#include "uzuki2/uzuki2.hpp"
uzuki2::validate(file_path, group_name);
```

This will raise an error if any violations of the specification are observed.
If a non-zero expected number of external objects is present:

```cpp
uzuki2::validate(file_path, group_name, num_externals);
```

Advanced users can also use the **uzuki2** parser to load the list into memory.
This is achieved by calling `parse()` with custom provisioner and external reference classes.
For example, [`tests/src/test_subclass.h`](tests/src/test_subclass.h) defines the `DefaultProvisioner` and `DefaultExternals` classes,
which can be used to load the HDF5 contents into `std::vector`s for easier downstream operations.

```cpp
DefaultExternals ext(nexpected);
auto ptr = uzuki2::parse<DefaultProvisioner>(file_path, group_name, ext);
```

Also see the [reference documentation](https://ltla.github.io/uzuki2) for more details.

### Building projects

If you're using CMake, you just need to add something like this to your `CMakeLists.txt`:

```
include(FetchContent)

FetchContent_Declare(
  libscran
  GIT_REPOSITORY https://github.com/LTLA/uzuki2
  GIT_TAG master # or any version of interest
)

FetchContent_MakeAvailable(uzuki2)
```

Then you can link to **uzuki** to make the headers available during compilation:

```
# For executables:
target_link_libraries(myexe uzuki2)

# For libaries
target_link_libraries(mylib INTERFACE uzuki2)
```

## Comparison to version 1

**uzuki2** involves some major changes from the original [**uzuki**](https://github.com/LTLA/uzuki) library.
Most obviously, we switched from JSON to HDF5.
The latter supports random access without loading the entire list contents into memory, 
which provides some optimization opportunities for parsers when large vectors are present.

In addition, arrays and data frames are no longer supported in **uzuki2**.
Such objects should instead be represented by external references,
under the assumption that any serialization framework using **uzuki2** would already have a separate mechanism for representing arrays and data frames. 
For example, the [**alabaster**](https://github.com/ArtifactDB/alabaster.base) framework has its own staging methods for these objects.

Just like the original **uzuki** library, we're just re-using the reference to [Uzuki Shimamura](https://myanimelist.net/character/70883/Uzuki_Shimamura) for the name:

![Uzuki Shimamura](https://media1.giphy.com/media/7Oy2FDqWV5mak/giphy.gif)
