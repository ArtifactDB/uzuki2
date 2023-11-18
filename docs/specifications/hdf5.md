# HDF5 Specification

## General comments

We use `**/` to represent a variable name of the group representing any of the supported R objects.
It is assumed that `**/` will be replaced by the actual name of the group in implementations,
as defined by users (for the top-level group) or by the specification (e.g., as a nested child of a list).

All objects should be nested inside an R list.

The top-level group may have a `uzuki_version` attribute, describing the version of the **uzuki2** specification that it uses.
This should be a scalar string dataset of the form `X.Y` for non-negative integers `X` and `Y`.
The latest version of this specification is **1.3**; if not provided, it is assumed to be **1.0**.

## Lists

An R list is represented as a HDF5 group (`**/`) with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"list"`.

This group should contain a subgroup `**/data` that contains the list elements.
Each list element is itself represented by a subgroup that is named after its 0-based position in the list, e.g., `**/data/0` for the first list element.
One subgroup should be present for each integer in `[0, N)`, given a list of length `N`.
Each list element may be any of the objects described in this specification, including further nested lists.

If the list is named, there will additionally be a 1-dimensional `**/names` string dataset of length equal to the number of elements in `**/data`.
See also the [comments on names](misc.md#comments-on-names).

## Atomic vectors

An atomic vector is represented as a HDF5 group (`**/`) with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"vector"`.
- `uzuki_type`, a scalar string dataset containing one of `"integer"`, `"boolean"`, `"number"` or `"string"`.
   - **(for version 1.0)** this may also be `"date"` or `"date-time"`.

The group should contain an 1-dimensional dataset at `**/data`.
Vectors of length 1 may also be represented as a scalar dataset.
(While R makes no distinction between scalars and length-1 vectors, this may be useful for other frameworks where this difference is relevant.)
The allowed HDF5 datatype depends on `uzuki_type`:

- `"integer"`, `"boolean"`: any type of `H5T_INTEGER` that can be represented by a 32-bit signed integer.
  Note that the converse is not required, i.e., the storage type does not need to be 32-bit if no such values are present in the dataset.
- **(for version < 1.3)** `"number"`: any type of `H5T_FLOAT` that can be represented by a double-precision float.
- **(for version >= 1.3)** `"number"`: any type of `H5T_FLOAT` or `H5T_INTEGER` that can be represented exactly by a double-precision (64-bit) float.
  This implies a limit of 32 bits for any integer datatype.
  See also the [HDF5 policy draft (v0.1.0)](https://github.com/ArtifactDB/Bioc-HDF5-policy/tree/v0.1.0) for more details.
- `"string"`: any type of `H5T_STRING` that can be represented by a UTF-8 encoded string.
- **(for version 1.0)** `"date"`: any type of `H5T_STRING` where the srings are in the `YYYY-MM-DD` format, or are equal to a missing placeholder value.
- **(for version 1.0)** `"date-time"`: any type of `H5T_STRING` where the srings are Internet Date/Time format, or are equal to a missing placeholder value.

For `boolean` type, values in `**/data` should be one of 0 (false) or 1 (true).

**(for versions >= 1.1)** 
For the `string` type, the group may optionally contain the `**/format` dataset.
This should be a scalar string dataset that specifies constraints to the format of the values in `**/data`:

- `"date"`: strings should be `YYYY-MM-DD` dates or the placeholder value.
- `"date-time"`: strings should be in the Internet Date/Time format ([RFC 3339, Section 5.6](https://www.rfc-editor.org/rfc/rfc3339#section-5.6)) or the placeholder value.

The atomic vector's group may also contain `**/names`, a 1-dimensional string dataset of length equal to that of `**/data`.
If `**/data` is a scalar, `**/names` should have length 1.
See also the [comments on names](misc.md#comments-on-names).

### Representing missing values

**(for version >= 1.1)** 
Each `**/data` dataset may optionally contain a `missing-value-placeholder` attribute.
If present, this should be a scalar dataset that specifies the placeholder for missing values.
Any value of `**/data` that is equal to this placeholder should be treated as missing.
If no such attribute is present, it can be assumed that there are no missing values. 

**(for version >= 1.2)** 
The data type of the placeholder attribute should be exactly the same as that of `**/data`, so as to avoid unexpected results upon casting.
The only exception is when `**/data` is a string, in which case the placeholder type may be of any string type;
it is expected that any comparison between the placeholder and strings in `**/data` will be performed bytewise in the same manner as `strcmp`.

**(for version == 1.1)** 
The data type of the placeholder attribute should have the same data type class as `**/data`.

**(for version >= 1.3)** 
Floating-point missingness should be identified using the equality operator when both the placeholder and data values are loaded into memory as IEEE754-compliant `double`s.
No casting should be performed to a lower-precision type, as this may cause a non-missing value to become equal to the placeholder.
If the placeholder is NaN, all NaNs in the dataset should be considered missing, regardless of the exact bit representation in the NaN payload.
See the [HDF5 policy draft (v0.1.0)](https://github.com/ArtifactDB/Bioc-HDF5-policy/tree/v0.1.0) for more details.

**(for version >= 1.1, < 1.3)** 
Floating-point missingness may be encoded in the payload of an NaN, which distinguishes it from a non-missing "not-a-number" value.
Comparisons on NaN placeholders should be performed in a bytewise manner (e.g., with `memcmp`) to ensure that the payload is taken into account.

**(for version 1.0)** 
Integer or boolean values of -2147483648 are treated as missing.
Missing floats are represented by [R's NA representation](https://github.com/wch/r-source/blob/869e0f734dc4971c420cf417f5e0d18c0974a5af/src/main/arithmetic.c#L90-L98).
For strings, each `**/data` dataset may contain a `missing-value-placeholder` attribute.
If present, this should be a scalar string dataset that specifies the placeholder for missing values.
Any value of `**/data` that is equal to this placeholder should be treated as missing.
If no such attribute is present, it can be assumed that there are no missing values. 

## Factors

A factor is represented as a HDF5 group (`**/`) with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"vector"`.
- `uzuki_type`, a scalar string dataset containing `"factor"`.
  - **(for version 1.0)** `uzuki_type` could also be set to `"ordered"`.
    This is the same as `uzuki_type` of `"factor"` with the `**/ordered` dataset set to a truthy value.

The group should contain an 1-dimensional dataset at `**/data`, containing 0-based indices into the levels.
This should be type of `H5T_INTEGER` that can be represented by a 32-bit signed integer.
Missing values are represented as described above for atomic vectors.

The group should also contain `**/levels`, a 1-dimensional string dataset that contains the levels for the indices in `**/data`.
Values in `**/levels` should be unique.
Values in `**/data` should be non-negative (missing values excepted) and less than the length of `**/levels`.
Note that the type constraints on `**/data` suggest that there should not be more than 2147483647 levels;
beyond that count, the levels cannot be indexed by elements of `**/data`.

The group may also contain `**/names`, a 1-dimensional string dataset of length equal to `data`.
See also the [comments on names](misc.md#comments-on-names).

**(for version >= 1.1)** The group may optionally contain `**/ordered`, a scalar integer dataset.
This should be interpreted as a boolean where a non-zero value specifies that we should assume that the levels are ordered.

## Nothing

A "nothing" (a.k.a., "null", "none") value is represented as a HDF5 group with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"nothing"`.

## External object

Each external object is represented as a HDF5 group (`**/`) with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"external"`.

This should contain an `**/index` scalar dataset, containing an index that identifies this external object uniquely within the entire list.
`**/index` should start at zero and be incremented whenever an external object is encountered. 

By indexing this external metadata, we can restore the object in its appropriate location in the list.
The exact mechanism by which this restoration occurs is implementation-defined.
