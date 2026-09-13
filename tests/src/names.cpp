
    // Works with names.
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("blub");
        write_strings(ghandle, "names", { "A", "B", "C", "D", "E" });
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::INTEGER);

        auto stuff = static_cast<const DefaultIntegerVector*>(parsed.get());
        EXPECT_TRUE(stuff->base.has_names);
        EXPECT_EQ(stuff->base.names.front(), "A");
        EXPECT_EQ(stuff->base.names.back(), "E");
    }


    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        write_numbers<int>(vhandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT);
        write_numbers(vhandle, "names", { "A", "B", "C", "D" });
    }
    expect_hdf5_error(path, "blub", "should be equal to the object length");
