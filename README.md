# R lists to HDF5 or JSON

![Unit tests](https://github.com/LTLA/uzuki2/actions/workflows/run-tests.yaml/badge.svg)
![Documentation](https://github.com/LTLA/uzuki2/actions/workflows/doxygenate.yaml/badge.svg)
[![codecov](https://codecov.io/gh/LTLA/uzuki2/branch/master/graph/badge.svg?token=J3dxS3MtT1)](https://codecov.io/gh/LTLA/uzuki2)

## Overview

The **uzuki2** repository describes a language-agnostic file format for serializing basic [R](https://r-project.org) lists.
List elements may be atomic vectors, `NULL`, or nested lists of such objects.
It also supports missing values in the vectors and per-element names on the vectors or lists.
A mechanism is also provided to handle external references to more complex objects (e.g., S4 classes) that cannot be directly saved into the format.

We support serialization in either [HDF5](https://www.hdfgroup.org/) or (possibly Gzip-compressed) [JSON](https://json.org).
Both of these are widely used formats and have complementary strengths for list representation.
HDF5 supports random access into list components, which can provide optimization opportunities when the list is large and/or contains large atomic vectors.
In contrast, JSON is easier to parse and has less storage overhead per list element.

## HDF5 Specification

We use `**/` to represent a variable name of the group representing any of the supported R objects.
It is assumed that `**/` will be replaced by the actual name of the group in implementations,
as defined by users (for the top-level group) or by the specification (e.g., as a nested child of a list).

All objects should be nested inside an R list.

### Lists

An R list is represented as a HDF5 group (`**/`) with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"list"`.

This group should contain a subgroup `**/data` that contains the list elements.
Each list element is itself represented by a subgroup that is named after its 0-based position in the list, e.g., `**/data/0` for the first list element.
Each list element may be any of the objects described in this specification, including further nested lists.

If the list is named, there will additionally be a 1-dimensional `**/names` string dataset of length equal to `uzuki_length`.

### Atomic vectors

An atomic vector is represented as a HDF5 group (`**/`) with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"vector"`.
- `uzuki_type`, a scalar string dataset containing one of `"integer"`, `"boolean"`, `"number"`, `"string"` or `"date"`.

The group should contain an 1-dimensional dataset at `**/data`.
Vectors of length 1 may also be represented as a scalar dataset.
(While R makes no distinction between scalars and length-1 vectors, this may be useful for other frameworks where this difference is relevant.)
The allowed HDF5 datatype depends on `uzuki_type`:

- `"integer"`, `"boolean"`: any type of `H5T_INTEGER` that can be represented by a 32-bit signed integer.
- `"number"`: any type of `H5T_FLOAT` that can be represented by a double-precision float.
- `"string"` or `"date"`: any type of `H5T_STRING` that can be represented by a UTF-8 encoded string.

For some `uzuki_type`, further considerations may be applicable:

- `"integer"`: values of `**/data` that are equal to -2147483648 should be treated as missing.
- `"boolean"`: values in `**/data` should be one of 0 (false), 1 (true), or -2147483648 (missing).
- `string`: the `**/data` dataset may contain a `"missing-value-placeholder"` attribute.
  If present, this should be a string scalar dataset that specifies the placeholder for missing values.
  Any value of `**/data` that is equal to this placeholder should be treated as missing.
- `"date"`: like `"string"`, the `**/data` dataset may contain a `missing-value-placeholder` attribute.
  The `**/data` dataset should only contain `YYYY-MM-DD` dates or the placeholder value.

The atomic vector's group may also contain `**/names`, a 1-dimensional string dataset of length equal to that of `**/data`.
If `**/data` is a scalar, `**/names` should have length 1.

### Factors

A factor is represented as a HDF5 group (`**/`) with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"vector"`.
- `uzuki_type`, a scalar string dataset containing one of `"factor"` or `"ordered"`.

The group should contain an 1-dimensional dataset at `**/data`.
This should be type of `H5T_INTEGER` that can be represented by a 32-bit signed integer.
Missing values are represented by -2147483648.

The group should also contain `**/levels`, a 1-dimensional string dataset that contains the levels for the indices in `data`.
Values in `**/levels` should be unique.
Values in `**/data` should be non-negative (missing values excepted) and less than the length of `**/levels`.

The group may also contain `**/names`, a 1-dimensional string dataset of length equal to `data`.

### Nothing

A "nothing" (a.k.a., "null", "none") value is represented as a HDF5 group with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"nothing"`.

### External object

Each external object is represented as a HDF5 group (`**/`) with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"external"`.

This should contain an `**/index` scalar dataset, containing an index that identifies this external object uniquely within the entire list.
`**/index` should start at zero and be incremented whenever an external object is encountered. 

By indexing this external metadata, we can restore the object in its appropriate location in the list.
The exact mechanism by which this restoration occurs is implementation-defined.

## JSON Specification

All R objects are represented by JSON objects with a `type` property.
Every R object should be nested inside an R list.

### Lists

An R list is represented as a JSON object with the following properties:

- `type`, set to `"list"`.
- `values`, an array of JSON objects corresponding to nested R objects.
  Each JSON object may follow any of the formats described in this specification.
- (optional) `"names"`, an array of length equal to `values`, containing the names of the list elements.

### Atomic vectors

An atomic vector is represented as a JSON object with the following properties:

- `type`, set to one of `"integer"`, `"boolean"`, `"number"`, `"string"` or `"date"`.
- `values`, an array of values for the vector (see below).
  This may also be a scalar of the same type as the array contents.
- (optional) `"names"`, an array of length equal to `values`, containing the names of the list elements.
  If `values` is a scalar, `names` should have length 1.

The contents of `values` is subject to some constraints:

- `"integer"`: values should be JSON numbers that can fit into a 32-bit signed integer.
  Missing values may be represented by `null` or the special value -2147483648.
- `"boolean"`: values should be JSON booleans or `null` (for missing values).
- `string`: values should be JSON strings.
  `null` is also allowed and represents a missing value.
- `"date"`: values should be JSON strings following a `YYYY-MM-DD` format.
  `null` is also allowed and represents a missing value.

Vectors of length 1 may also be represented as scalars of the appropriate type.
While R makes no distinction between scalars and length-1 vectors, this may be useful for other frameworks where this difference is relevant.

### Factors

A factor is represented as a JSON object with the following properties:

- `type`, set to one of `"factor"` or `"ordered"`.
- `values`, an array of integer indices for the factor.
  These should be non-negative JSON numbers that can fit into a 32-bit signed integer.
  They should also be less than the length of `levels`.
  Missing values may be represented by `null` or the special value -2147483648.
- `levels`, an array of unique strings containing the levels for the indices in `values`.
- (optional) `"names"`, an array of length equal to `values`, containing the names of the list elements.

### Nothing

A "nothing" (a.k.a., "null", "none") value is represented as a JSON object with the following properties:

- `type`, set to `"nothing"`.

### External object

Each external object is represented as a JSON object with the following properties:

- `type`, set to `"index"`.
- `index`, a non-negative JSON number that can fit into a 32-bit signed integer.
  This identifies this external object uniquely within the entire list.
  See the equivalent in the HDF5 specification for more details.

## Comments on names

Both HDF5 and JSON support naming of the vector elements, typically via the `names` group/property.
If `names` are supplied, their contents should always be non-missing (e.g., not `null` in JSON, no `missing-value-placeholder` in HDF5).
Each name is allowed to be any string, including an empty string.

It is technically permitted to provide duplicate names in `names`, consistent with how R itself supports duplicate names in its lists and vectors.
However, this is not recommended as other frameworks may wish to use representations that assume unique names, e.g., using Python dictionaries to represent named lists.
By providing unique names, users can improve interoperability with native data structures in other frameworks.

## Validation

### Quick start

A reference implementation of the validator is provided as a header-only C++ library in [`include/uzuki2`](include/uzuki2).
This is useful for portable deployment in different frameworks like R, Python, etc.
We can check that a JSON/HDF5 file complies with the **uzuki** specification:

```cpp
#include "uzuki2/uzuki2.hpp"
uzuki2::validate_hdf5(h5_file_path, h5_group_name);
uzuki2::validate_json(json_file_path);
```

This will raise an error if any violations of the specification are observed.
If a non-zero expected number of external objects is present:

```cpp
uzuki2::validate_hdf5(h5_file_path, h5_group_name, num_externals);
```

Advanced users can also use the **uzuki2** parser to load the list into memory.
This is achieved by calling `parse()` with custom provisioner and external reference classes.
For example, [`tests/src/test_subclass.h`](tests/src/test_subclass.h) defines the `DefaultProvisioner` and `DefaultExternals` classes,
which can be used to load the HDF5 contents into `std::vector`s for easier downstream operations.

```cpp
DefaultExternals ext(nexpected);
auto ptr = uzuki2::parse_hdf5<DefaultProvisioner>(file_path, group_name, ext);
```

Also see the [reference documentation](https://ltla.github.io/uzuki2) for more details.

### Building projects

If you're using CMake, you just need to add something like this to your `CMakeLists.txt`:

```
include(FetchContent)

FetchContent_Declare(
  uzuki2 
  GIT_REPOSITORY https://github.com/LTLA/uzuki2
  GIT_TAG master # or any version of interest
)

FetchContent_MakeAvailable(uzuki2)
```

Then you can link to **uzuki2** to make the headers available during compilation:

```
# For executables:
target_link_libraries(myexe uzuki2)

# For libaries
target_link_libraries(mylib INTERFACE uzuki2)
```

You will probably also need to link to the HDF5 and/or Zlib libraries, if you haven't done so already.
For system installations of both libraries, you can just use `find_package()`: 

```
find_package(HDF5 REQUIRED COMPONENTS C CXX)
target_link_libraries(myexe hdf5::hdf5 hdf5::hdf5_cpp)

find_package(ZLIB)
target_link_libraries(myexe ZLIB::ZLIB)
```

## Comparison to version 1

**uzuki2** involves some major changes from the original [**uzuki**](https://github.com/LTLA/uzuki) library.
Most obviously, we added support for HDF5 alongside the JSON format.
The latter supports random access without loading the entire list contents into memory, 
which provides some optimization opportunities for parsers when large vectors are present.

Arrays and data frames are no longer supported in **uzuki2**.
Such objects should instead be represented by external references,
under the assumption that any serialization framework using **uzuki2** would already have a separate mechanism for representing arrays and data frames. 
For example, the [**alabaster**](https://github.com/ArtifactDB/alabaster.base) framework has its own staging methods for these objects.

In the JSON format, **uzuki2** is also more explicit with its serialization of lists.
These now have their own dedicated `"type": "list"`, rather than relying on the implicit interpretation of arrays as unnamed lists and JSON objects as named lists.
In particular, treating JSON objects as named lists led to ambiguities when a list element was named `"type"`; it also failed to preserve the ordering of list elements.

Just like the original **uzuki** library, we're just re-using the reference to [Uzuki Shimamura](https://myanimelist.net/character/70883/Uzuki_Shimamura) for the name:

![Uzuki Shimamura](https://media1.giphy.com/media/7Oy2FDqWV5mak/giphy.gif)
