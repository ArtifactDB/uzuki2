# Comparison to original 

**uzuki2** involves some major changes from the original [**uzuki**](https://github.com/LTLA/uzuki) library.
Most obviously, we added support for HDF5 alongside the JSON format.
The latter supports random access without loading the entire list contents into memory, 
which provides some optimization opportunities for parsers when large vectors are present.

Arrays and data frames are no longer supported in **uzuki2**.
Such objects should instead be represented by external references,
under the assumption that any serialization framework using **uzuki2** would already have a separate mechanism for representing arrays and data frames. 
For example, the [**alabaster**](https://github.com/ArtifactDB/alabaster.base) framework has its own staging methods for these objects.

In the JSON format, **uzuki2** is also more explicit with its serialization of lists.
These now have their own dedicated `"type": "list"`, rather than relying on the implicit interpretation of arrays as unnamed lists and JSON objects as named lists.
In particular, treating JSON objects as named lists led to ambiguities when a list element was named `"type"`; it also failed to preserve the ordering of list elements.
