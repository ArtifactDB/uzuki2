#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_hdf5.hpp"

#include "test_subclass.h"
#include "utils.h"

TEST(Hdf5Format, Error) {
    const auto path = "TEST-format.h5";
    std::vector<std::string> data{ "harry", "ron", "hermoine" };

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "string");
        add_version(vhandle, "1.1");
        write_string(vhandle, "format", "foobar");
        write_strings(vhandle, "data", data);
    }
    expect_hdf5_error(path, "foo", "unsupported format");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "string");
        add_version(vhandle, "1.1");
        write_strings(vhandle, "format", { "foobar" });
        write_strings(vhandle, "data", data);
    }
    expect_hdf5_error(path, "foo", "scalar");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "string");
        add_version(vhandle, "1.1");
        vhandle.createDataSet("format", H5::PredType::NATIVE_INT32, H5S_SCALAR);
        write_strings(vhandle, "data", data);
    }
    expect_hdf5_error(path, "foo", "can be represented by a UTF-8 encoded string");
}

TEST(Hdf5Format, LegacyType) {
    const auto path = "TEST-format.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "date");
        add_version(vhandle, "1.1");
        write_strings(vhandle, "data", { "2077-12-12" });
    }
    expect_hdf5_error(path, "foo", "unknown vector type");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "date-time");
        add_version(vhandle, "1.1");
        write_strings(vhandle, "data", { "2077-12-12T00:00:00Z" });
    }
    expect_hdf5_error(path, "foo", "unknown vector type");
}
