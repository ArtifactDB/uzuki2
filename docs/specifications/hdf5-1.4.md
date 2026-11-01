

# HDF5 Specification (1.4)

## Overview 

This document contains the **uzuki2** specification for storing an R-style list in a HDF5 file.

## General comments

### File organization

The top-level R list is the list that is not nested within any other list.
It may contain any number of nested R objects.

The top-level R list is represented by a HDF5 group within the HDF5 file, following the ["List" specification](#list). 
The name of this group is left to the implementation.
The group may even be nested in other groups, as long as those groups do not represent **uzuki2** R lists.
We do not mandate a specific name or location to provide some flexibility in how applications store one or more R lists.

Each R object inside the top-level R list is represented by its own HDF5 group.
This group should be contained within the HDF5 group corresponding to the top-level R list.

### Datatypes

This specification will declare the largest HDF5 datatype that can be used for a particular That isdataset/attribute.
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

Some R objects may contain names for its elements.
Each name is allowed to be any string, including an empty string.
Duplicate names are technically permitted, consistent with how R itself supports duplicate names in its lists and vectors.
However, be aware that other frameworks may wish to use representations that assume unique names, e.g., using Python dictionaries to represent named lists.
Unique names are preferred as this improves interoperability with those data structures. 

### Miscellaneous

The HDF5 group corresponding to the top-level R list should have a `uzuki_version` attribute, describing the version of the **uzuki2** specification that it uses.
This attribute should hold a scalar string dataset containing the value "1.4".

Unless explicitly mentioned in this specification, the parametrization of HDF5 properties is left to the implementation.
This includes dataset properties like chunking and compression, or file properties like page size and fill time and value. 

## Object types

### List

A list is represented as a HDF5 group with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"list"`.

The list's group should contain a `data` group that contains the list elements.
The $i$-th list element should be represented by the `data/{i}` group for zero-based $i$, e.g., `data/0` for the first element.
The number of groups inside `data` should be equal to the number of list elements, i.e., the length of the list.
Each list element may be any of the R object types described in this specification, including further nested list objects.

Optionally, the list's group may contain `names`, a 1-dimensional string dataset of length equal to the number of list elements. 
This contains the names of the list, i.e., the $i$-th element of the `names` dataset is the name of the $i$-th list element.

### Atomic vector

An atomic vector is represented as a HDF5 group with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"vector"`.
- `uzuki_type`, a scalar string dataset containing one of `"integer"`, `"boolean"`, `"number"` or `"string"`.

The group should contain `data`, a 1-dimensional dataset.
The length of this dataset defines the length of the atomic vector.
The set of allowed HDF5 datatypes for this dataset depends on the value of `uzuki_type`:

- `"integer"`: a HDF5 integer datatype that can be exactly represented by a 32-bit signed integer.
- `"boolean"`: a HDF5 integer datatype that can be exactly represented by a 32-bit signed integer.
  Values should be one of 0 (false) or non-zero (true).
- `"number"`: a HDF5 integer or float datatype that can be represented exactly by a double-precision (64-bit) float.
- `"string"`: a HDF5 string datatype. 
  <!--- -->

An atomic vector of length 1 may also be represented as a scalar dataset.
While R makes no distinction between scalars and length-1 vectors, this may be useful for other frameworks where this difference is relevant.

Optionally, for the `string` type, the atomic vector's group may contain `format`.
This should be a scalar string dataset that specifies constraints to the format of the strings in `data`:

- `"date"`: strings should be `YYYY-MM-DD` dates or the placeholder value.
- `"date-time"`: strings should be in the Internet Date/Time format ([RFC 3339, Section 5.6](https://www.rfc-editor.org/rfc/rfc3339#section-5.6)) or the placeholder value.

Optionally, the atomic vector's group may contain `names`, a 1-dimensional string dataset of length equal to that of the atomic vector.
This contains the names of the atomic vector, i.e., the $i$-th element of the `names` dataset is the name of the $i$-th atomic vector element.
If the `data` dataset is scalar, the `names` dataset should have length 1.

#### Representing missing values

Optionally, the `data` dataset may contain a `missing-value-placeholder` scalar attribute.
This specifies the placeholder that is used to represent missing values in the dataset.
Any value of `data` that is equal to this placeholder should be treated as missing.
This approach avoids the need to store a separate mask dataset and is more flexible than a fixed sentinel that might conflict with the dataset contents. 

The datatype of the placeholder attribute should be exactly the same as that of the `data` dataset.
This avoids the risk of changing values upon casting between types prior to the equality comparison.
The only exception is for strings , in which case the placeholder may be of any valid string datatype (see comments in ["Datatypes"](#datatypes)).
All strings in the dataset with the same sequence of bytes (as defined by, e.g., `strcmp`) as the placeholder value should be treated as missing.

Floating-point missingness should be identified by comparing the placeholder and data values as IEEE754-compliant `double`s.
No casting should be performed to a lower-precision type prior to the equality comparison, as this may cause a non-missing value to become equal to the placeholder.
If the placeholder is not NaN, the comparison can be done with the usual `==` operator.
If the placeholder is NaN, all NaNs in the dataset should be considered missing, regardless of the exact bit representation in the NaN payload.
We do not consider the NaN payload during placeholder comparisons as this may not be handled reliably across machines.



If the `data` dataset does not have a placeholder attribute, it can be assumed that there are no missing values.

### Factor

A factor is represented as a HDF5 group with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"vector"`.
- `uzuki_type`, a scalar string dataset containing `"factor"`.

The factor's group should contain `data`, a 1-dimensional dataset containing factor codes.
The `data` dataset should use a HDF5 integer datatype that can be represented by a 32-bit signed integer.
(Admittedly, this should have been an unsigned integer, but we started with a signed integer and we'll just keep it so for back-compatibility.)
The length of this dataset defines the length of the factor.

Optionally, a factor of length 1 may be represented as a scalar `data` dataset.
While R makes no distinction between scalars and length-1 vectors, this may be useful for other frameworks where this difference is relevant.

The factor's group should contain `levels`, a 1-dimensional string dataset that contains the factor levels. 
Entries of this dataset should be unique.

Codes in the `data` dataset should be interpreted as 0-based indices into the `levels` dataset, i.e., a code of $i$ refers to the $i$-th factor level.
Each code should be non-negative and less than the length of `levels`, or equal to the missing value placeholder if the `missing-value-placeholder` attribute exists.
Note that the datatype constraints on `data` imply that there should not be more than 2147483648 levels;
beyond that, the levels cannot be indexed by elements of `data`.

Optionally, the `data` dataset may contain a `missing-value-placeholder` scalar attribute.
This specifies the placeholder that is used to represent missing codes in the dataset.
Any value of `data` that is equal to this placeholder should be treated as missing.
The datatype of the placeholder attribute should be exactly the same as that of the `data` dataset.

Optionally, the factor's group may contain `names`, a 1-dimensional string dataset of length equal to that of the  `data` dataset.
This contains the names of the factor, i.e., the $i$-th element of the `names` dataset is the name of the $i$-th factor element.
If the `data` dataset is a scalar, the `names` dataset should have length 1.



### Compressed variable length string array

A CVLS array is functionally equivalent to an atomic vector of strings.
However, it uses the compressed variable-length string (CVLS) format for greater storage efficiency in the presence of highly variable string lengths.
This is represented as a HDF5 group with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"vector"`.
- `uzuki_type`, a scalar string dataset containing `"vls"`.

This group should contain:

- `data`, a 1-dimensional dataset of a compound datatype of 2 members named `offset` and `length`.
  Each member should be of a datatype that can be represented by an unsigned 64-bit integer.
- `heap`, a 1-dimensional dataset of unsigned 8-bit integers.
  This contains bytes from all of the strings in the CVLS array, e.g., by concatenating all strings.

The $i$-th entry of `data` refers to a slice $[o_i, o_i + l_i)$ of the `heap` dataset,
where $o_i$ is the zero-based offset and $l_i$ is the length in bytes from the `offset` and `length` members, respectively.
This defines a variable-length UTF-8-encoded string, terminated at the first null in the slice.
If no null character is present, the entire slice should be used as the contents of the string consists of the entire slice. 

Entries of `data` may be sorted in any order and may refer to overlapping or non-contiguous slices.
The only requirement is that each `[offset, offset + length)` slice lies within the boundaries of the heap dataset.

No restrictions are placed on the contents of the `heap` dataset, as long as the strings can be recovered from slices as described above.
Writers of the `heap` dataset may concatenate strings in arbitrary order, merge duplicate strings, add padding between strings, etc.
The heap may even contain unused bytes, i.e., not referenced by any slice in `data`.

Optionally, a CVLS array of length 1 may be represented as a scalar `data` dataset of the aforementioned compound datatype.
While R makes no distinction between scalars and length-1 vectors, this may be useful for other frameworks where this difference is relevant.

Optionally, a `missing-value-placeholder` scalar string attribute on the `data` dataset may be present, defining a placeholder for missing values.
An entry of `data` should be considered as missing if its corresponding string is equal to the placeholder.

Optionally, the group may contain `names`, a 1-dimensional string dataset of length equal to that of the `data` dataset.
This contains the names of the CVLS array, i.e., the $i$-th element of the `names` dataset is the name of the $i$-th CVLS array element.
If `data` is a scalar, `names` should have length 1.

#### Further comments

The CVLS format addresses HDF5's inability to efficiently handle arrays of variable-length strings.
Using a fixed-length datatype requires padding all strings to the longest string, causing an increase in disk usage that cannot be completely offset by compression;
while HDF5's own variable-length string datatype does not compress the strings themselves, only the pointers to those strings.
CVLS stores variable-length strings directly without any padding, and the `heap` dataset can be chunked and compressed to improve storage efficiency.

Some care is required to avoid inadvertent casts to/from a `char` of platform-specific signedness to the heap's unsigned integer type.
Recall that both ASCII and UTF-8 define a character in terms of its binary representation, e.g., `0b01000001` for `A`.
Thus, implementations should ensure that the binary representation of each character is preserved when reading or writing to the heap. 
We recommend following these procedures:

- For reading, we first read the bytes into an `uint8_t` array via `H5::DataSet::read()` with `NATIVE_UINT8`.
  We then access those bytes via an aliased `char*` to extract each VLS.
- For writing, we create an aliased `char*` pointer to a `uint8_t` array and copy each VLS into this array via the aliased pointer.
  We then use `H5::DataSet::write()` with `NATIVE_UINT8` to write the `uint8_t` array. 

This approach ensures that each byte is faithfully transferred between the HDF5 file and memory as a `uint8_t` with a well-defined and portable binary representation.
The aliasing pointer then interprets the byte as a `char` with the same sequence of bits, regardless of the platform's `char` signedness.

When writing a CVLS array, we recommend that entries in the same chunk of the `data` dataset refer to neighboring parts of the `heap` dataset.
This allows readers to efficiently extract bytes from the latter when iterating through chunks of the former.

### Null

A null (a.k.a., "nothing", "none") is represented as a HDF5 group with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"nothing"`.

### External object

An external object is represented as a HDF5 group with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"external"`.

The external object's group should contain an `index` scalar dataset of any datatype that can be represented by a 32-bit signed integer.
The dataset contains an index that identifies this external object uniquely within the top-level R list.
By indexing some external metadata with the value of `index`, we can restore the external object at its appropriate location within the list.
The exact mechanism by which this restoration occurs is implementation-defined.

Given a top-level R list, the set of all of `index` values should be $(0, 1, ..., N - 1)$ where $N$ is the total number of external objects.
That is, each `index` value should be used exactly once, starting from zero and incrementing for each external object in the top-level list.
