#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_hdf5.hpp"

#include "utils.h"

TEST(Hdf5Error, ForceList) {
    auto path = "TEST-other.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        std::map<std::string, std::string> attrs;
        attrs["uzuki_object"] = "nothing";
        super_group_opener(handle, "whee", attrs);
    }
    expect_hdf5_error(path, "whee", "top-level object should represent an R list");
}

TEST(Hdf5Error, UnknownType) {
    auto path = "TEST-other.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        std::map<std::string, std::string> attrs;
        attrs["uzuki_object"] = "blah";
        super_group_opener(handle, "whee", attrs);
    }
    expect_hdf5_error(path, "whee", "unknown uzuki2 object");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "whee", "BLAH");
        write_numbers<std::int32_t>(vhandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT32);
    }
    expect_hdf5_error(path, "whee", "unknown vector type");
}

TEST(Hdf5Error, WrongShape) {
    auto path = "TEST-other.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        const hsize_t shape [] = { 10, 20 };
        H5::DataSpace dspace(2, shape);
        vhandle.createDataSet("data", H5::PredType::NATIVE_INT32, dspace);
    }
    expect_hdf5_error(path, "blub", "scalar or 1-dimensional");
}

