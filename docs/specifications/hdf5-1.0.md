

# HDF5 Specification (1.0)

## Comments

### General

Every R object is represented by a HDF5 group.
In the descriptions below, we use `**/` as a placeholder for the name of the group.

All R objects should be nested inside an R list.
In other words, the top-level HDF5 group should represent an R list.

The top-level group should have a `uzuki_version` attribute, describing the version of the **uzuki2** specification that it uses.
This attribute should hold a scalar string dataset containing the value "1.0".
If not present, the version is assumed to be "1.0" for back-compatibility purposes.
This should use a datatype that can be represented by a UTF-8 encoded string.



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
  This should use a datatype that can be represented by a UTF-8 encoded string.

This group should contain a subgroup `**/data` that contains the list elements.
Each list element is itself represented by a subgroup that is named after its 0-based position in the list, e.g., `**/data/0` for the first list element.
One subgroup should be present for each integer in `[0, N)`, given a list of length `N`.
Each list element may be any of the objects described in this specification, including further nested lists.

If the list is named, there will additionally be a 1-dimensional `**/names` string dataset of length equal to the number of elements in `**/data`.
This should use a datatype that can be represented by a UTF-8 encoded string.

### Atomic vectors

An atomic vector is represented as a HDF5 group (`**/`) with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"vector"`.
  This should use a datatype that can be represented by a UTF-8 encoded string.
- `uzuki_type`, a scalar string dataset containing one of `"integer"`, `"boolean"`, `"number"`, `"string"`, `"date"` or `"date-time"`.
  This should use a datatype that can be represented by a UTF-8 encoded string.

The group should contain an 1-dimensional dataset at `**/data`.
Vectors of length 1 may also be represented as a scalar dataset.
(While R makes no distinction between scalars and length-1 vectors, this may be useful for other frameworks where this difference is relevant.)
The allowed HDF5 datatype for `**/data` depends on `uzuki_type`:

- `"integer"`, `"boolean"`: a HDF5 integer datatype that can be exactly represented by a 32-bit signed integer.
  Note that the converse is not required, i.e., the datatype does not need to be 32-bit if no such values are present in the dataset.
- `"number"`: a HDF5 float datatype that can be exactly represented by a double-precision float.
- `"string"`: a HDF5 string datatype that can be represented by a UTF-8 encoded string.
- `"date"`: a HDF5 string datatype that can be represented by a UTF-8 encoded string,
  where the strings are in the `YYYY-MM-DD` format or are equal to a missing placeholder value.
- `"date-time"`: a HDF5 string datatype that can be represented by a UTF-8 encoded string,
  where the strings are Internet Date/Time format, or are equal to a missing placeholder value.

For `boolean` type, values in `**/data` should be one of 0 (false) or non-zero (true).



The atomic vector's group may also contain `**/names`, a 1-dimensional string dataset of length equal to that of `**/data`.
This should use a datatype that can be represented by a UTF-8 encoded string.
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
  This should use a datatype that can be represented by a UTF-8 encoded string.
- `uzuki_type`, a scalar string dataset containing `"factor"` or `"ordered"`.
  This should use a datatype that can be represented by a UTF-8 encoded string.

The group should contain an 1-dimensional dataset at `**/data`, containing 0-based indices into the levels.
Vectors of length 1 may also be represented as a scalar dataset.
(While R makes no distinction between scalars and length-1 vectors, this may be useful for other frameworks where this difference is relevant.)

The `**/data` dataset should use a HDF5 integer datatype that can be represented by a 32-bit signed integer.
Admittedly, this should have been an unsigned integer, but we started with a signed integer and we'll just keep it so for back-compatibility.

The group should contain `**/levels`, a 1-dimensional string dataset that contains the levels for the indices in `**/data`.
This should use a datatype that can be represented by a UTF-8 encoded string.
Values in `**/levels` should be unique.

Values in `**/data` should be non-negative (missing value placeholders excepted) and less than the length of `**/levels`.
Note that the datatype constraints on `**/data` suggest that there should not be more than 2147483647 levels,
as beyond that, the levels cannot be indexed by elements of `**/data`.

Missing values in the factor are represented by a placeholder, as described [above](representing-missing-values) for atomic integer vectors.


The group may also contain `**/names`, a 1-dimensional string dataset of length equal to `data`.
This should use a datatype that can be represented by a UTF-8 encoded string.
If `**/data` is a scalar, `**/names` should have length 1.





### Nothing

A "nothing" (a.k.a., "null", "none") value is represented as a HDF5 group with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"nothing"`.
  This should use a datatype that can be represented by a UTF-8 encoded string.

### External object

Each external object is represented as a HDF5 group (`**/`) with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"external"`.
  This should use a datatype that can be represented by a UTF-8 encoded string.

This should contain an `**/index` scalar dataset, containing an index that identifies this external object uniquely within the entire list.
`**/index` should start at zero and be incremented whenever an external object is encountered. 

By indexing some external metadata with the value of `**/index`, we can restore the external object in its appropriate location in the R list.
The exact mechanism by which this restoration occurs is implementation-defined.
