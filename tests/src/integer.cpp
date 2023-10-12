#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_hdf5.hpp"

#include "test_subclass.h"
#include "utils.h"

TEST(Hdf5IntegerTest, SimpleLoading) {
    auto path = "TEST-integer.h5";

    // Simple stuff works correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        create_dataset<int>(vhandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
        auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
        EXPECT_EQ(iptr->size(), 5);
        EXPECT_EQ(iptr->base.values.front(), 1);
        EXPECT_EQ(iptr->base.values.back(), 5);
        EXPECT_FALSE(iptr->base.scalar);
    }

    // Works with names.
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("blub");
        create_dataset(ghandle, "names", { "A", "B", "C", "D", "E" });
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::INTEGER);

        auto stuff = static_cast<const DefaultIntegerVector*>(parsed.get());
        EXPECT_TRUE(stuff->base.has_names);
        EXPECT_EQ(stuff->base.names.front(), "A");
        EXPECT_EQ(stuff->base.names.back(), "E");
    }

    // Scalars work correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        write_scalar(vhandle, "data", 999, H5::PredType::NATIVE_INT);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
        auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
        EXPECT_EQ(iptr->size(), 1);
        EXPECT_EQ(iptr->base.values.front(), 999);
        EXPECT_TRUE(iptr->base.scalar);
    }
}

TEST(Hdf5NumberTest, BlockLoading) {
    auto path = "TEST-string.h5";

    // Buffer size is 10000, so we make sure we have enough values to go through a few iterations.
    std::vector<int> collected(25000);
    for (size_t i = 0; i < collected.size(); ++i) {
        collected[i] = i;
    }

    // Uncompressed works correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        create_dataset<int>(vhandle, "data", collected, H5::PredType::NATIVE_INT, /* compressed */ false);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
        auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
        EXPECT_EQ(iptr->base.values, collected);
    }

    // Compressed works correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        create_dataset<int>(vhandle, "data", collected, H5::PredType::NATIVE_INT, /* compressed */ true);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
        auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
        EXPECT_EQ(iptr->base.values, collected);
    }
}

TEST(Hdf5IntegerTest, MissingValues) {
    auto path = "TEST-integer.h5";

    // Simple stuff works correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        create_dataset<int>(vhandle, "data", { 1, 2, -2147483648, 4, 5 }, H5::PredType::NATIVE_INT);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
        auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
        EXPECT_EQ(iptr->size(), 5);
        EXPECT_EQ(iptr->base.values[2], -123456789); // i.e., the test's missing value placeholder.
    }

    // Latest version doesn't automatically use -2**31 to be a placeholder.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        add_version(vhandle, "1.1");
        create_dataset<int>(vhandle, "data", { 1, 2, -2147483648, 4, 5 }, H5::PredType::NATIVE_INT);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
        auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
        EXPECT_EQ(iptr->size(), 5);
        EXPECT_EQ(iptr->base.values[2], -2147483648); 
    }

    // We can instead set our own placeholder.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        add_version(vhandle, "1.1");

        auto dhandle = create_dataset<int>(vhandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT);
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT, H5S_SCALAR);
        int placeholder = 3;
        ahandle.write(H5::PredType::NATIVE_INT, &placeholder);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
        auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
        EXPECT_EQ(iptr->size(), 5);
        EXPECT_EQ(iptr->base.values[2], -123456789); 
    }
}

TEST(Hdf5IntegerTest, CheckError) {
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
        auto vhandle = vector_opener(handle, "blub", "integer");
        create_dataset<int>(vhandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT);
        create_dataset(vhandle, "names", { "A", "B", "C", "D" });
    }
    expect_hdf5_error(path, "blub", "should be equal to length");
}

TEST(JsonIntegerTest, SimpleLoading) {
    // Simple stuff works correctly.
    {
        auto parsed = load_json("{ \"type\": \"integer\", \"values\": [ 0, 1000, -1, 12345, -2e+4 ] }");
        EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
        auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
        EXPECT_EQ(iptr->size(), 5);
        EXPECT_FALSE(iptr->base.scalar);
        EXPECT_EQ(iptr->base.values[0], 0);
        EXPECT_EQ(iptr->base.values[1], 1000);
        EXPECT_EQ(iptr->base.values[2], -1);
        EXPECT_EQ(iptr->base.values[3], 12345);
        EXPECT_EQ(iptr->base.values[4], -20000);
    }

    // Works with names.
    {
        auto parsed = load_json("{ \"type\": \"integer\", \"values\": [ 0, 1000, -1, 12345, -2e+4 ], \"names\": [ \"a\", \"bb\", \"ccc\", \"dddd\", \"eeeee\" ] }");
        EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
        auto stuff = static_cast<const DefaultIntegerVector*>(parsed.get());
        EXPECT_TRUE(stuff->base.has_names);
        EXPECT_EQ(stuff->base.names.front(), "a");
        EXPECT_EQ(stuff->base.names.back(), "eeeee");
    }

    // Works with scalars.
    {
        auto parsed = load_json("{ \"type\": \"integer\", \"values\": 1234 }");
        EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
        auto stuff = static_cast<const DefaultIntegerVector*>(parsed.get());
        EXPECT_TRUE(stuff->base.scalar);
        EXPECT_EQ(stuff->base.values[0], 1234);
    }
}

TEST(JsonIntegerTest, MissingValues) {
    {
        auto parsed = load_json("{ \"type\": \"integer\", \"values\": [ 0, 1000, -1, null, -2e+4 ] }");
        EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
        auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
        EXPECT_EQ(iptr->size(), 5);
        EXPECT_EQ(iptr->base.values[3], -123456789); // i.e., the test's missing value placeholder.
    }

    // Same for our special value.
    {
        auto parsed = load_json("{ \"type\": \"integer\", \"values\": [ 0, 1000, -2147483648, null, -2e+4 ] }");
        EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
        auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
        EXPECT_EQ(iptr->size(), 5);
        EXPECT_EQ(iptr->base.values[2], -123456789); 
        EXPECT_EQ(iptr->base.values[3], -123456789);
    }

    // Except in the latest version.
    {
        auto parsed = load_json("{ \"type\": \"integer\", \"values\": [ 0, 1000, -2147483648, null, -2e+4 ], \"version\":\"1.1\" }");
        EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
        auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
        EXPECT_EQ(iptr->size(), 5);
        EXPECT_EQ(iptr->base.values[2], -2147483648); 
        EXPECT_EQ(iptr->base.values[3], -123456789); 
    }
}

TEST(JsonIntegerTest, CheckError) {
    expect_json_error("{ \"type\": \"integer\" }", "expected 'values' property");
    expect_json_error("{ \"type\": \"integer\", \"values\": \"foo\"}", "expected a number");

    expect_json_error("{ \"type\": \"integer\", \"values\": [true]}", "expected a number");
    expect_json_error("{ \"type\": \"integer\", \"values\": [1.2]}", "expected an integer");
    expect_json_error("{ \"type\": \"integer\", \"values\": [-999999999999]}", "cannot be represented");

    expect_json_error("{ \"type\": \"integer\", \"values\": [-99], \"names\": true}", "expected an array");
    expect_json_error("{ \"type\": \"integer\", \"values\": [-99], \"names\": [\"a\", \"b\"]}", "should be the same");
}
