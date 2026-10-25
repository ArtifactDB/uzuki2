

# HDF5 Specification (1.0)

## Comments

### General

Every R object is represented by a HDF5 group.
In the specification for each object type (see below), we use `**/` as a placeholder for the name of its group.

All R objects should be nested inside an R list.
In other words, the top-level HDF5 group should represent an R list.

The top-level group should have a `uzuki_version` attribute, describing the version of the **uzuki2** specification that it uses.
This attribute should hold a scalar string dataset containing the value "1.0".
If not present, the version is assumed to be "1.0" for back-compatibility purposes.

Unless explicitly mentioned here, the parametrization of HDF5 properties is left to the implementation.
This includes dataset properties like chunking and compression, or file properties like page size and fill time and value. 

### Datatypes

The specification will declare the largest HDF5 datatype that can be used for a particular dataset/attribute.
The aim is to guarantee that implementations can safely use the declared type to represent the contents of the dataset/attribute without any overflow or loss of precision.
This avoids the need to implement multiple code paths for different types when reading the data from the HDF5 file.
Writers are still free to choose a smaller type to reduce the size of the dataset/attribute on disk.

If a dataset/attribute contains integers, the specification should declare the bit width and signedness of the largest acceptable integer datatype. 
We assume a two's complement representation for signed integers.
For example, most objects containing integers will specify that the corresponding HDF5 dataset/attribute's datatype is no larger than a 32-bit signed integer. 
Readers can thus safely assume that an `int32_t` is sufficient to represent all possible values.
Writers may create HDF5 datasets with any integer datatype that can be fully represented by an `int32_t`, e.g., `H5T_NATIVE_INT8`, `H5T_NATIVE_UINT16`.

If a dataset/attribute contains floating-point numbers, the specification should declare the bit width of the largest acceptable float datatype. 
We assume that all representations are IEEE754-compliant, to ensure the faithful propagation of special values like infinity and NaN.
For example, most objects containing floats will specify that the corresponding HDF5 dataset/attribute's datatype is no larger than a 64-bit IEEE754-compliant float.
Readers can thus safely assume that an `double` is sufficient to represent all values in memory.
Writers may create HDF5 datasets with any floating-point or integer datatype that can be exactly represented by a `double`, e.g., `H5T_NATIVE_FLOAT`, `H5T_NATIVE_UINT32`.

If a dataset/attribute contains strings, any string datatype may be used subject to the following considerations:

- The datatype uses a UTF-8 encoding.
  This implicitly supports ASCII, which is a subset of UTF-8.
- Both variable and fixed length datatypes may be used.
  For fixed-length datatypes, the length of each string is defined as the number of bytes to the first null character, or as the fixed length if no null character is present.
  For variable-length datatypes, each entry of the dataset/attribute should refer to a null-terminated string, i.e., readers should never encounter a `NULL` pointer.
- Both null termination and null padding may be used, as both are considered to yield appropriate C-style strings when the length is unknown.
  Fortran-style blank-padded strings should not be used as it is unclear whether the trailing blanks are meant to be discarded.

### Names 

Some R objects may have a `**/names` dataset in their HDF5 group.
Each name is allowed to be any string, including an empty string.

It is technically permitted to provide duplicate names in `**/names`, consistent with how R itself supports duplicate names in its lists and vectors.
However, be aware that other frameworks may wish to use representations that assume unique names, e.g., using Python dictionaries to represent named lists.
Unique names are preferred as this improves interoperability with those data structures. 

## Object types

### Lists

An R list is represented as a HDF5 group (`**/`) with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"list"`.

This group should contain a subgroup `**/data` that contains the list elements.
Each list element is itself represented by a subgroup that is named after its 0-based position in the list, e.g., `**/data/0` for the first list element.
One subgroup should be present for each integer in `[0, N)`, given a list of length `N`.
Each list element may be any of the objects described in this specification, including further nested lists.

If the list is named, there will additionally be a 1-dimensional `**/names` string dataset of length equal to the number of elements in `**/data`.

### Atomic vectors

An atomic vector is represented as a HDF5 group (`**/`) with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"vector"`.
- `uzuki_type`, a scalar string dataset containing one of `"integer"`, `"boolean"`, `"number"`, `"string"`, `"date"` or `"date-time"`.

The group should contain an 1-dimensional dataset at `**/data`.
Vectors of length 1 may also be represented as a scalar dataset.
(While R makes no distinction between scalars and length-1 vectors, this may be useful for other frameworks where this difference is relevant.)
The allowed HDF5 datatype for `**/data` depends on `uzuki_type`:

- `"integer"`, `"boolean"`: a HDF5 integer datatype that can be exactly represented by a 32-bit signed integer.
  Note that the converse is not required, i.e., the datatype does not need to be 32-bit if no such values are present in the dataset.
- `"number"`: a HDF5 float datatype that can be exactly represented by a double-precision (64-bit) float.
- `"string"`: a HDF5 string datatype. 
- `"date"`: a HDF5 string datatype. 
  Each string should be in the `YYYY-MM-DD` format, or be equal to a missing placeholder value.
- `"date-time"`: a HDF5 string datatype. 
  Each string should be in the Internet Date/Time format, or be equal to a missing placeholder value.

For `boolean` type, values in `**/data` should be one of 0 (false) or non-zero (true).



The atomic vector's group may also contain `**/names`, a 1-dimensional string dataset of length equal to that of `**/data`.
If `**/data` is a scalar, `**/names` should have length 1.

#### Representing missing values







Integer or boolean values of -2147483648 are treated as missing.
Missing floats are represented by [R's NA representation](https://github.com/wch/r-source/blob/869e0f734dc4971c420cf417f5e0d18c0974a5af/src/main/arithmetic.c#L90-L98).
For strings, each `**/data` dataset may contain a `missing-value-placeholder` attribute.
If present, this should be a scalar string dataset that specifies the placeholder for missing values.
Any value of `**/data` that is equal to this placeholder should be treated as missing.
If no such attribute is present, it can be assumed that there are no missing values.



### Factors

A factor is represented as a HDF5 group (`**/`) with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"vector"`.
- `uzuki_type`, a scalar string dataset containing `"factor"` or `"ordered"`.

The group should contain an 1-dimensional dataset at `**/data`, containing 0-based indices into the levels.
Vectors of length 1 may also be represented as a scalar dataset.
(While R makes no distinction between scalars and length-1 vectors, this may be useful for other frameworks where this difference is relevant.)

The `**/data` dataset should use a HDF5 integer datatype that can be represented by a 32-bit signed integer.
Admittedly, this should have been an unsigned integer, but we started with a signed integer and we'll just keep it so for back-compatibility.

The group should contain `**/levels`, a 1-dimensional string dataset that contains the levels for the indices in `**/data`.
Values in `**/levels` should be unique.

Values in `**/data` should be non-negative (missing value placeholders excepted) and less than the length of `**/levels`.
Note that the datatype constraints on `**/data` suggest that there should not be more than 2147483647 levels,
as beyond that, the levels cannot be indexed by elements of `**/data`.

Missing values in the factor are represented by a placeholder, as described [above](representing-missing-values) for atomic integer vectors.


The group may also contain `**/names`, a 1-dimensional string dataset of length equal to `data`.
If `**/data` is a scalar, `**/names` should have length 1.





### Nothing

A "nothing" (a.k.a., "null", "none") value is represented as a HDF5 group with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"nothing"`.

### External object

Each external object is represented as a HDF5 group (`**/`) with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"external"`.

This should contain an `**/index` scalar dataset, containing an index that identifies this external object uniquely within the entire list.
This should use a datatype that can be represented by a 32-bit signed integer.
Each value of `**/index` should be used exactly once through the list,
and the set of all `**/index` values in a single list should be `(0, 1, ..., N - 1)` where `N` is the total number of external objects in the list.

By indexing some external metadata with the value of `**/index`, we can restore the external object in its appropriate location in the R list.
The exact mechanism by which this restoration occurs is implementation-defined.
