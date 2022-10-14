# R lists to HDF5

## Overview

The **uzuki2** repository describes a format for safely serializing R lists into HDF5.
In particular, there are some subtleties with respect to preserving type, e.g., factors, dates, dimension names.
Some mechanism is also required to handle multi-dimensional arrays and external references to non-serializable objects.
The C++ library implements a portable validator for this specification. 

## Specification

### Lists

An R list is represented as a HDF5 group with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"list"`.
- `uzuki_length`, a scalar integer dataset specifying the length of the list.

Each list element is represented by a subgroup that is named after its 0-based position in the list.
The contents of each element's group may be any of the objects listed in this specifiction, including further lists.

If the list is named, it will additionally contain a 1-dimensional `names` string dataset of length equal to `uzuki_length`.

### Atomic vectors/arrays

An atomic vector (or array) is represented as a HDF5 group with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"atomic"`.
- `uzuki_type`, a scalar string dataset containing one of `"integer"`, `"boolean"` `"float"`, `"string"`, `"date"`, `"factor"` or `"ordered"`.

The group should contain an any-dimensional `data` dataset.
The allowed datatype class depends on `uzuki_type`:

- `H5T_INTEGER` for `"integer"`, `"boolean"`, `"factor"` or `"ordered"`.
- `H5T_FLOAT` for `"float"`.
- `H5T_STRING` for `"string"`, `"date"`.

`data` itself may have several optional attributes:

- `uzuki_missing`: a scalar dataset of the same datatype class as `data`, which specifies the value in `data` used to represent missing values.
  If none is specified, it defaults to -2147483648 for `H5T_INTEGER`, the IEEE `NaN` for `H5T_FLOAT`, and `"NA"` for `H5T_STRING`.
- `uzuki_force1d`: a scalar integer dataset where a non-zero value indicates that `data` should be treated a 1-dimensional array in R rather than as a vector.
  This is only used if `data` is 1-dimensional, and if absent, it is assumed that a 1D HDF5 array should be treated as an R vector.

For some `uzuki_type`, further constraints may be applicable:

- `"boolean"`: values in `data` should be one of 0 (false), 1 (true), or the `uzuki_missing` value.
- `"factor"` or `"ordered"`: the atomic vector's group should also contain `levels`.
  This is a scalar string dataset that contains the levels for the indices in `data`.
  Values in `data` should be non-negative and less than the length of `levels`;
  except for missing values, which are represented by `uzuki_missing`.
- `"date"`, the `data` dataset should only contain `YYYY-MM-DD` dates or the `uzuki_missing` value.

The atomic vector's group may also contain `names`.
This is a group containing 1-dimensional string datasets numbered after the dimensions in `data`.
Each dataset is of length equal to the extent of the corresponding dimension in `data` and contains the names along that dimension.
If a dataset is missing, it is assumed that the dimension is unnamed.

Note that the fastest changing dimension in HDF5 should also be treated as the fastest changing dimension in R.
However, HDF5 orders its dimensions from slowest to fastest, while R orders from fastest to slowest;
thus, the identities of the dimensions is reversed when mapping to/from R and HDF5.

### Null

A null value is represented as a HDF5 group with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"null"`.

### External object

Each external object is represented as a HDF5 group with the following attributes:

- `uzuki_object`, a scalar string dataset containing the value `"other"`.

This should contain an `index` scalar dataset, containing an index that identifies this external object uniquely within the entire list.
`index` should start at zero and be incremented whenever an external object is encountered. 

By indexing this external metadata, we can restore the object in its appropriate location in the list.
The exact mechanism by which this restoration occurs is implementation-defined.
