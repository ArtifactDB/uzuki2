

TEST(Hdf5IntegerTest, ShapeError) {
    auto path = "TEST-integer.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        write_number(vhandle, "data", 999, H5::PredType::NATIVE_INT32);
    }

}
