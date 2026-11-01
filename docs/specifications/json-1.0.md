

# JSON Specification (1.0)

## Overview 

This document contains the **uzuki2** specification for storing an R-style list in a JSON file.

## General comments

The top-level R list is the list that is not nested within any other list.
It may contain any number of nested R objects.

The top-level list is represented by a JSON object following the ["List" specification](#list). 
This JSON object should also have a `version` property that contains the **uzuki2** specification version. 
This should be a string containing the value "1.0".
If not present, the version is assumed to be "1.0" for back-compatibility purposes.

Each R object is represented by its own JSON object with a `type` property.
This should be contained within the JSON object corresponding to the top-level list.

Some R objects may contain names for its elements.
Each name is allowed to be any string, including an empty string.
Duplicate names are technically permitted, consistent with how R itself supports duplicate names in its lists and vectors.
However, be aware that other frameworks may wish to use representations that assume unique names, e.g., using Python dictionaries to represent named lists.
Unique names are preferred as this improves interoperability with those data structures. 

## Object types

### List

A list is represented as a JSON object with the following properties:

- `type`, set to `"list"`.
- `values`, an array of JSON objects, each of which represents a nested R object.
  The $i$-th entry represents the $i$-th list element.
  Each JSON object may follow any of the formats described in this specification.
- (optional) `"names"`, an array of strings of length equal to that of `values`.
  The $i$-th entry contains the name of the $i$-th list element.

### Atomic vector

An atomic vector is represented as a JSON object with the following properties:

- `type`, set to one of `"integer"`, `"boolean"`, `"number"`, 
  `"string"`, `"date"`, or `"date-time"`. The last two specify strings in the date or Internet Date/Time format.
- `values`, an array of values for the atomic vector (see below).
  The length of this array defines the length of the atomic vector, where the $i$-th entry of the arry is the $i$-th atomic vector element.
  This may also be a scalar of the same type as the array contents.
- (optional) `"names"`, an array of strings of length equal to that of `values`.
  The $i$-th entry of this array contains the name of the $i$-th atomic vector element.
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

### Factor

A factor is represented as a JSON object with the following properties:

- `type`, set to `"factor"` or `"ordered"`.
- `values`, an array of 0-based factor codes.
  The length of this array defines the factor length, and the $i$-th entry is the code of the $i$-th factor element.
  Each entry should be a non-negative JSON number that can fit into a 32-bit signed integer.
  Each entry should also be less than the length of `levels`.
  Missing values are represented by `null`.
  Missing integers may also be represented by the special value -2147483648.
- `levels`, an array of unique strings containing the levels for the indices in `values`.
- (optional) `"names"`, an array of length equal to that of `values`. 
  The $i$-th entry of this array contains the name of the $i$-th factor element.
 <!--- -->

### Null 

A null (a.k.a., "nothing", "none") value is represented as a JSON object with the following properties:

- `type`, set to `"nothing"`.

### External object

An external object is represented as a JSON object with the following properties:

- `type`, set to `"external"`.
- `index`, a non-negative JSON number that can fit into a 32-bit signed integer.

The value of `index` identifies this external object uniquely within the top-level R list.
By indexing some external metadata with the value of `index`, we can restore the external object in its appropriate location in the R list.
The exact mechanism by which this restoration occurs is implementation-defined.

Each value of `index` should be used exactly once throughout the top-level list.
The set of all `index` values throughout a top-level list should be $(0, 1, ..., N - 1)$ where $N$ is the total number of external objects in the top-level list.
