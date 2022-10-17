#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse.hpp"

#include "test_subclass.h"
#include "utils.h"

TEST(MiscTest, CheckError) {
    auto path = "TEST-date.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        handle.createGroup("whee");
    }
    expect_hdf5_error(path, "whee", "uzuki_object");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        std::map<std::string, std::string> attrs;
        attrs["uzuki_object"] = "blah";
        super_group_opener(handle, "whee", attrs);
    }
    expect_hdf5_error(path, "whee", "unknown uzuki2 object");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        std::map<std::string, std::string> attrs;
        attrs["uzuki_object"] = "vector";
        super_group_opener(handle, "whee", attrs);
    }
    expect_hdf5_error(path, "whee", "uzuki_type");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "whee", "BLAH");
        create_dataset<int>(vhandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT);
    }
    expect_hdf5_error(path, "whee", "unknown vector type");
}
