
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "string");
        add_version(vhandle, "1.1");
        write_string(vhandle, "format", "foobar");
        create_dataset(vhandle, "data", { "harry", "ron", "hermoine" });
    }
    expect_hdf5_error(path, "foo", "unsupported format");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "string");
        add_version(vhandle, "1.1");
        vhandle.createDataSet("format", H5::PredType::NATIVE_INT, H5S_SCALAR);
        create_dataset(vhandle, "data", { "harry", "ron", "hermoine" });
    }
    expect_hdf5_error(path, "foo", "can be represented by a UTF-8 encoded string");


    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "date");
        add_version(vhandle, "1.1");
        create_dataset(vhandle, "data", { "2077-12-12" });
    }
    expect_hdf5_error(path, "foo", "unknown vector type");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "date-time");
        add_version(vhandle, "1.1");
        create_dataset(vhandle, "data", { "2077-12-12T00:00:00Z" });
    }
    expect_hdf5_error(path, "foo", "unknown vector type");

