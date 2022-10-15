# R lists to HDF5

## Overview

The **uzuki2** repository describes a format for safely serializing R lists into HDF5.
In particular, there are some subtleties with respect to preserving type, e.g., factors, dates, dimension names.
Some mechanism is also required to handle multi-dimensional arrays and external references to non-serializable objects.
The C++ library implements a portable validator for this specification. 

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
- `uzuki_type`, a scalar string dataset containing one of `"integer"`, `"boolean"` `"float"`, `"string"`, `"date"`, `"factor"` or `"ordered"`.

The group should contain an 1-dimensional dataset at `**/data`.
The allowed datatype class depends on `uzuki_type`:

- `H5T_INTEGER` for `"integer"`, `"boolean"`, `"factor"` or `"ordered"`.
- `H5T_FLOAT` for `"float"`.
- `H5T_STRING` for `"string"`, `"date"`.

For some `uzuki_type`, further considerations may be applicable:

- `"integer"`: values of `**/data` that are equal to -2147483648 should be treated as missing.
- `"boolean"`: values in `**/data` should be one of 0 (false), 1 (true), or -2147483648 (missing).
- `"factor"` or `"ordered"`: the atomic vector's group should also contain `**/levels`.
  This is a scalar string dataset that contains the levels for the indices in `data`.
  Values in `data` should be non-negative and less than the length of `**/levels`;
  except for missing values, which are represented by `uzuki_missing`.
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

- `uzuki_object`, a scalar string dataset containing the value `"other"`.

This should contain an `**/index` scalar dataset, containing an index that identifies this external object uniquely within the entire list.
`**/index` should start at zero and be incremented whenever an external object is encountered. 

By indexing this external metadata, we can restore the object in its appropriate location in the list.
The exact mechanism by which this restoration occurs is implementation-defined.
