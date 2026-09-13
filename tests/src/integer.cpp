#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_hdf5.hpp"

#include "test_subclass.h"
#include "utils.h"

TEST(Hdf5IntegerTest, Vector) {
    auto path = "TEST-integer.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        write_numbers<int>(vhandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT32);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
    auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
    EXPECT_EQ(iptr->size(), 5);
    EXPECT_EQ(iptr->base.values.front(), 1);
    EXPECT_EQ(iptr->base.values.back(), 5);
    EXPECT_FALSE(iptr->base.scalar);
}

TEST(Hdf5IntegerTest, Scalar) {
    auto path = "TEST-integer.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        write_number(vhandle, "data", 999, H5::PredType::NATIVE_INT32);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
    auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
    EXPECT_EQ(iptr->size(), 1);
    EXPECT_EQ(iptr->base.values.front(), 999);
    EXPECT_TRUE(iptr->base.scalar);
}

TEST(Hdf5IntegerTest, ChunkStream) {
    auto path = "TEST-integer.h5";

    // Simulate multiple chunks so that we test the while{} loop for streaming values.
    const std::size_t nlen = 25000;
    std::vector<int> collected(nlen);
    for (std::size_t i = 0; i < nlen; ++i) {
        collected[i] = i;
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        write_numbers(vhandle, "data", collected, H5::PredType::NATIVE_INT32, /* chunk_size = */ 82);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
    auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
    EXPECT_EQ(iptr->base.values, collected);
}

TEST(Hdf5IntegerTest, ForbiddenType) {
    auto path = "TEST-forbidden.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        write_numbers<int>(vhandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_UINT32);
    }
    expect_hdf5_error(path, "blub", "cannot be represented");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        write_numbers<int>(vhandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT64);
    }
    expect_hdf5_error(path, "blub", "cannot be represented by 32-bit");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "integer");
        write_numbers<double>(ghandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_DOUBLE);
    }
    expect_hdf5_error(path, "foo", "dataset cannot be represented by 32-bit");

    // Strings are obviously not allowed.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        write_strings(vhandle, "data", { "foo", "bar" });
    }
    expect_hdf5_error(path, "blub", "cannot be represented by 32-bit");
}

TEST(Hdf5IntegerTest, SmallerType) {
    auto path = "TEST-integer.h5";

    // Smaller types are allowed as long as they fit in the 32-bit signed integer.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        write_numbers<int>(vhandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_UINT16);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
    auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
    EXPECT_EQ(iptr->base.values[0], 1);
    EXPECT_EQ(iptr->base.values[4], 5);
}

TEST(Hdf5IntegerTest, LegacyMissing) {
    auto path = "TEST-integer.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        write_numbers<int>(vhandle, "data", { 1, 2, -2147483648, 4, 5 }, H5::PredType::NATIVE_INT32);
    }
    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
    auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
    EXPECT_EQ(iptr->size(), 5);
    EXPECT_EQ(iptr->base.values[2], -123456789); // i.e., the test's missing value placeholder.
}

TEST(Hdf5IntegerTest, MissingPlaceholder) {
    auto path = "TEST-integer.h5";

    // Latest version doesn't automatically use -2**31 to be a placeholder.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        add_version(vhandle, "1.1");
        write_numbers<int>(vhandle, "data", { 1, 2, -2147483648, 4, 5 }, H5::PredType::NATIVE_INT32);
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
        auto dhandle = write_numbers<int>(vhandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT32);
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT32, H5S_SCALAR);
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

TEST(Hdf5IntegerTest, MissingPlaceholderError) {
    auto path = "TEST-integer.h5";

    // Format version < 1.2 only needs the same type class.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "integer");
        add_version(ghandle, "1.1");
        auto dhandle = write_numbers<double>(ghandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT32);
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR);
    }
    expect_hdf5_error(path, "foo", "same type class as");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "integer");
        add_version(ghandle, "1.1");
        auto dhandle = write_numbers<double>(ghandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT32);
        constexpr hsize_t one = 1;
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT32, H5::DataSpace(1, &one));
    }
    expect_hdf5_error(path, "foo", "scalar");

    // Format version >= 1.2 requires the exact same datatype.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "integer");
        add_version(ghandle, "1.2");
        auto dhandle = write_numbers<double>(ghandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_UINT8);
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT8, H5S_SCALAR);
    }
    expect_hdf5_error(path, "foo", "same type as");
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
