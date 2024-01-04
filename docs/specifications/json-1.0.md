

# JSON Specification (1.0)

## Comments

### General

All R objects are represented by JSON objects with a `type` property.
Every R object should be nested inside an R list, i.e., the top-level JSON object should represent an R list.

The top-level object should have a `version` property that contains the **uzuki2** specification version. 
This should be a string containing the value "1.0".
If not present, the version is assumed to be "1.0" for back-compatibility purposes.

### Names 

Some R objects may have a `names` property in the JSON object.
If `names` is supplied, its contents should always be non-missing.
Each name is allowed to be any string, including an empty string.

It is technically permitted to provide duplicate names in `names`, consistent with how R itself supports duplicate names in its lists and vectors.
However, this is not recommended as other frameworks may wish to use representations that assume unique names, e.g., using Python dictionaries to represent named lists.
By providing unique names, users can improve interoperability with native data structures in other frameworks.

## Object types

### Lists

An R list is represented as a JSON object with the following properties:

- `type`, set to `"list"`.
- `values`, an array of JSON objects corresponding to nested R objects.
  Each JSON object may follow any of the formats described in this specification.
- (optional) `"names"`, an array of length equal to `values`, containing the names of the list elements.

### Atomic vectors

An atomic vector is represented as a JSON object with the following properties:

- `type`, set to one of `"integer"`, `"boolean"`, `"number"`, `"string"`.
  - **(for version 1.0)** `type` could also be set to `"date"` or `"date-time"`.
    This specifies strings in the date or Internet Date/Time format.
- `values`, an array of values for the vector (see below).
  This may also be a scalar of the same type as the array contents.
- (optional) `"names"`, an array of length equal to `values`, containing the names of the list elements.
  If `values` is a scalar, `names` should have length 1.

The contents of `values` is subject to some constraints:

- `"number"`: values should be JSON numbers. 
  Missing values are represented by `null`.
  IEEE special values can be represented by strings, i.e., `NaN`, `Inf`, `-Inf`.
- `"integer"`: values should be JSON numbers that can be represented by a 32-bit signed integer.
  Missing values may be represented by `null`.
  Missing integers may also be represented by the special value -2147483648.
- `"boolean"`: values should be JSON booleans or `null` (for missing values).
- `string`: values should be JSON strings.
  `null` is also allowed and represents a missing value.



Vectors of length 1 may also be represented as scalars of the appropriate type.
While R makes no distinction between scalars and length-1 vectors, this may be useful for other frameworks where this difference is relevant.

### Factors

A factor is represented as a JSON object with the following properties:

- `type`, set to `"factor"` or `"ordered"`.
- `values`, an array of 0-based integer indices for the factor.
  These should be non-negative JSON numbers that can fit into a 32-bit signed integer.
  They should also be less than the length of `levels`.
  Missing values are represented by `null`.
  Missing integers may also be represented by the special value -2147483648.
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

By indexing some external metadata with the value of `index`, we can restore the external object in its appropriate location in the R list.
The exact mechanism by which this restoration occurs is implementation-defined.
