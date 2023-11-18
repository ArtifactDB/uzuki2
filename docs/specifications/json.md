# JSON Specification

## General comments

All R objects are represented by JSON objects with a `type` property.
Every R object should be nested inside an R list.

The top-level object may have a `version` property that contains the **uzuki2** specification version as a `"X.Y"` string for non-negative integers `X` and `Y`.
The latest version of this specification is **1.2**; if missing, the version can be assumed to be **1.0**.

## Lists

An R list is represented as a JSON object with the following properties:

- `type`, set to `"list"`.
- `values`, an array of JSON objects corresponding to nested R objects.
  Each JSON object may follow any of the formats described in this specification.
- (optional) `"names"`, an array of length equal to `values`, containing the names of the list elements.
  See also the [comments on names](misc.md#comments-on-names).

## Atomic vectors

An atomic vector is represented as a JSON object with the following properties:

- `type`, set to one of `"integer"`, `"boolean"`, `"number"`, `"string"`.
  - **(for version 1.0)** `type` could also be set to `"date"` or `"date-time"`.
    This specifies strings in the date or Internet Date/Time format.
- `values`, an array of values for the vector (see below).
  This may also be a scalar of the same type as the array contents.
- (optional) `"names"`, an array of length equal to `values`, containing the names of the list elements.
  If `values` is a scalar, `names` should have length 1.
  See also the [comments on names](misc.md#comments-on-names).

The contents of `values` is subject to some constraints:

- `"number"`: values should be JSON numbers. 
  Missing values are represented by `null`.
  IEEE special values can be represented by strings, i.e., `NaN`, `Inf`, `-Inf`.
- `"integer"`: values should be JSON numbers that can be represented by a 32-bit signed integer.
  Missing values may be represented by `null`.
  - **(for version 1.0)** missing integers could also be represented by the special value -2147483648.
- `"boolean"`: values should be JSON booleans or `null` (for missing values).
- `string`: values should be JSON strings.
  `null` is also allowed and represents a missing value.

**(for version >= 1.1)** 
For `type` of `"string"`, the object may optionally have a `format` property that constrains the `values`:
  
- `"date"`: values should be JSON strings following a `YYYY-MM-DD` format.
  `null` is also allowed and represents a missing value.
- `"date-time"`: values should be JSON strings following the Internet Date/Time format.
  `null` is also allowed and represents a missing value.

Vectors of length 1 may also be represented as scalars of the appropriate type.
While R makes no distinction between scalars and length-1 vectors, this may be useful for other frameworks where this difference is relevant.

## Factors

A factor is represented as a JSON object with the following properties:

- `type`, set to `"factor"`. 
  - **(for version 1.0)** `type` can also be set to `"ordered"` for ordered levels.
- `values`, an array of 0-based integer indices for the factor.
  These should be non-negative JSON numbers that can fit into a 32-bit signed integer.
  They should also be less than the length of `levels`.
  Missing values are represented by `null`.
  - **(for version 1.0)** missing values could also be represented by the special value -2147483648.
- `levels`, an array of unique strings containing the levels for the indices in `values`.
- (optional) `"names"`, an array of length equal to `values`, containing the names of the list elements.
  See also the [comments on names](misc.md#comments-on-names).
- **(for version >= 1.1)** (optional) `ordered`, a boolean indicating whether to assume that the levels are ordered.
  If absent, levels are assumed to be non-ordered.

## Nothing

A "nothing" (a.k.a., "null", "none") value is represented as a JSON object with the following properties:

- `type`, set to `"nothing"`.

## External object

Each external object is represented as a JSON object with the following properties:

- `type`, set to `"index"`.
- `index`, a non-negative JSON number that can fit into a 32-bit signed integer.
  This identifies this external object uniquely within the entire list.
  See the equivalent in the HDF5 specification for more details.
