#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse.hpp"

#include "test_subclass.h"
#include "utils.h"

TEST(IntegerTest, SimpleLoading) {
    auto path = "TEST-integer.h5";

    // Simple stuff works correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        create_dataset<int>(vhandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT);
    }
    {
        auto parsed = uzuki2::parse<DefaultProvisioner>(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
        auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
        EXPECT_EQ(iptr->size(), 5);
        EXPECT_EQ(iptr->base.values.front(), 1);
        EXPECT_EQ(iptr->base.values.back(), 5);
    }

    // Works with names.
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("blub");
        create_dataset(ghandle, "names", { "A", "B", "C", "D", "E" });
    }
    {
        auto parsed = uzuki2::parse<DefaultProvisioner>(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::INTEGER);

        auto stuff = static_cast<const DefaultIntegerVector*>(parsed.get());
        EXPECT_TRUE(stuff->base.has_names);
        EXPECT_EQ(stuff->base.names.front(), "A");
        EXPECT_EQ(stuff->base.names.back(), "E");
    }
}

TEST(IntegerTest, MissingValues) {
    auto path = "TEST-integer.h5";

    // Simple stuff works correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        create_dataset<int>(vhandle, "data", { 1, 2, -2147483648, 4, 5 }, H5::PredType::NATIVE_INT);
    }
    {
        auto parsed = uzuki2::parse<DefaultProvisioner>(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
        auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
        EXPECT_EQ(iptr->size(), 5);
        EXPECT_EQ(iptr->base.values[2], -123456789); // i.e., the test's missing value placeholder.
    }
}

TEST(IntegerTest, CheckError) {
    auto path = "TEST-integer.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "integer");
        ghandle.createGroup("data");
    }
    expect_hdf5_error(path, "foo", "expected a dataset at 'foo/data'");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "integer");
        create_dataset<double>(ghandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_DOUBLE);
    }
    expect_hdf5_error(path, "foo", "expected an integer dataset at 'foo/data'");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "integer");
        write_scalar(ghandle, "data", 1, H5::PredType::NATIVE_DOUBLE);
    }
    expect_hdf5_error(path, "foo", "1-dimensional");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        create_dataset<int>(vhandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT);
        create_dataset(vhandle, "names", { "A", "B", "C", "D" });
    }
    expect_hdf5_error(path, "blub", "should be equal to length");
}
