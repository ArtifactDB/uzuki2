

# HDF5 Specification (1.2)

## Comments

### General

Every R object is represented by a HDF5 group.
In the descriptions below, we use `**/` as a placeholder for the name of the group.

All R objects should be nested inside an R list.
In other words, the top-level HDF5 group should represent an R list.

The top-level group should have a `uzuki_version` attribute, describing the version of the **uzuki2** specification that it uses.
This attribute should hold a scalar string dataset containing the value "1.2".



### Names 

Some R objects may have a `**/names` dataset in their HDF5 group.
If `**/names` is supplied, the contents should always be non-missing, so any `missing-value-placeholder` will not be respected.
Each name is allowed to be any string, including an empty string.

It is technically permitted to provide duplicate names in `**/names`, consistent with how R itself supports duplicate names in its lists and vectors.
However, this is not recommended as other frameworks may wish to use representations that assume unique names, e.g., using Python dictionaries to represent named lists.
By providing unique names, users can improve interoperability with native data structures in other frameworks.

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
- `uzuki_type`, a scalar string dataset containing one of `"integer"`, `"boolean"`, `"number"` or `"string"`.

The group should contain an 1-dimensional dataset at `**/data`.
Vectors of length 1 may also be represented as a scalar dataset.
(While R makes no distinction between scalars and length-1 vectors, this may be useful for other frameworks where this difference is relevant.)
The allowed HDF5 datatype depends on `uzuki_type`:

- `"integer"`, `"boolean"`: any type of `H5T_INTEGER` that can be represented by a 32-bit signed integer.
  Note that the converse is not required, i.e., the storage type does not need to be 32-bit if no such values are present in the dataset.
- `"number"`: any type of `H5T_FLOAT` or `H5T_INTEGER` that can be represented exactly by a double-precision (64-bit) float.
- `"string"`: any type of `H5T_STRING` that can be represented by a UTF-8 encoded string.


For `boolean` type, values in `**/data` should be one of 0 (false) or non-zero (true).

For the `string` type, the group may optionally contain the `**/format` dataset.
This should be a scalar string dataset that specifies constraints to the format of the values in `**/data`:

- `"date"`: strings should be `YYYY-MM-DD` dates or the placeholder value.
- `"date-time"`: strings should be in the Internet Date/Time format ([RFC 3339, Section 5.6](https://www.rfc-editor.org/rfc/rfc3339#section-5.6)) or the placeholder value.

The atomic vector's group may also contain `**/names`, a 1-dimensional string dataset of length equal to that of `**/data`.
If `**/data` is a scalar, `**/names` should have length 1.

### Representing missing values

Each `**/data` dataset may optionally contain a `missing-value-placeholder` attribute.
If present, this should be a scalar dataset that specifies the placeholder for missing values.
Any value of `**/data` that is equal to this placeholder should be treated as missing.
If no such attribute is present, it can be assumed that there are no missing values.The data type of the placeholder attribute should be exactly the same as that of `**/data`, so as to avoid unexpected results upon casting.
The only exception is when `**/data` is a string, in which case the placeholder type may be of any string type;
it is expected that any comparison between the placeholder and strings in `**/data` will be performed bytewise in the same manner as `strcmp`.The data type of the placeholder attribute should have the same data type class as `**/data`.Floating-point missingness may be encoded in the payload of an NaN, which distinguishes it from a non-missing "not-a-number" value.
Comparisons on NaN placeholders should be performed in a bytewise manner (e.g., with `memcmp`) to ensure that the payload is taken into account.

### Factors

A factor is represented as a HDF5 group (`**/`) with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"vector"`.
- `uzuki_type`, a scalar string dataset containing `"factor"`.

The group should contain an 1-dimensional dataset at `**/data`, containing 0-based indices into the levels.
This should be type of `H5T_INTEGER` that can be represented by a 32-bit signed integer.
(Admittedly, this should have been an unsigned integer, but we started with a signed integer and we'll just keep it so for back-compatibility.)
Missing values are represented as described above for atomic vectors.

The group should also contain `**/levels`, a 1-dimensional string dataset that contains the levels for the indices in `**/data`.
Values in `**/levels` should be unique.
Values in `**/data` should be non-negative (missing values excepted) and less than the length of `**/levels`.
Note that the type constraints on `**/data` suggest that there should not be more than 2147483647 levels;
beyond that count, the levels cannot be indexed by elements of `**/data`.

The group may also contain `**/names`, a 1-dimensional string dataset of length equal to `data`.
See also the [comments on names](misc.md#comments-on-names).



### Nothing

A "nothing" (a.k.a., "null", "none") value is represented as a HDF5 group with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"nothing"`.

### External object

Each external object is represented as a HDF5 group (`**/`) with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"external"`.

This should contain an `**/index` scalar dataset, containing an index that identifies this external object uniquely within the entire list.
`**/index` should start at zero and be incremented whenever an external object is encountered. 

By indexing some external metadata with the value of `**/index`, we can restore the external object in its appropriate location in the R list.
The exact mechanism by which this restoration occurs is implementation-defined.
