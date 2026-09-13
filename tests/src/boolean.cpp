#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_hdf5.hpp"

#include "test_subclass.h"
#include "utils.h"

TEST(Hdf5BooleanTest, Vector) {
    auto path = "TEST-boolean.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "boolean");
        write_numbers<int>(vhandle, "data", { 1, 0, 1, 0, 0 }, H5::PredType::NATIVE_INT32);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::BOOLEAN);
    auto bptr = static_cast<const DefaultBooleanVector*>(parsed.get());
    EXPECT_EQ(bptr->size(), 5);
    EXPECT_EQ(bptr->base.values.front(), 1);
    EXPECT_EQ(bptr->base.values.back(), 0);
    EXPECT_FALSE(bptr->base.scalar);
}

TEST(Hdf5BooleanTest, Scalar) {
    auto path = "TEST-boolean.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "boolean");
        write_number(vhandle, "data", 1, H5::PredType::NATIVE_INT32);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::BOOLEAN);
    auto bptr = static_cast<const DefaultBooleanVector*>(parsed.get());
    EXPECT_EQ(bptr->size(), 1);
    EXPECT_EQ(bptr->base.values.front(), 1);
    EXPECT_TRUE(bptr->base.scalar);
}

TEST(Hdf5BooleanTest, ChunkStream) {
    auto path = "TEST-boolean.h5";

    // Simulate multiple chunks so that we test the while{} loop for streaming values.
    const std::size_t len = 25000;
    std::vector<unsigned char> collected(len);
    for (std::size_t i = 0; i < len; ++i) {
        collected[i] = i % 2;
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "boolean");
        write_numbers(vhandle, "data", collected, H5::PredType::NATIVE_INT32, /* chunk_size = */ 379);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::BOOLEAN);
    auto iptr = static_cast<const DefaultBooleanVector*>(parsed.get());
    EXPECT_EQ(iptr->base.values, collected);
}

TEST(Hdf5BooleanTest, ForbiddenType) {
    auto path = "TEST-forbidden.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "boolean");
        write_numbers<int>(vhandle, "data", { 0, 1, 0, 0, 1 }, H5::PredType::NATIVE_UINT32);
    }
    expect_hdf5_error(path, "blub", "cannot be represented");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "boolean");
        write_numbers<int>(vhandle, "data", { 0, 1, 0, 0, 1 }, H5::PredType::NATIVE_INT64);
    }
    expect_hdf5_error(path, "blub", "cannot be represented by 32-bit");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "boolean");
        write_numbers<double>(ghandle, "data", { 0, 1, 0, 0, 1 }, H5::PredType::NATIVE_DOUBLE);
    }
    expect_hdf5_error(path, "foo", "dataset cannot be represented by 32-bit");

    // Strings are obviously not allowed.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "boolean");
        write_strings(vhandle, "data", { "foo", "bar" });
        add_version(handle.openGroup("blub"), "1.3");
    }
    expect_hdf5_error(path, "blub", "cannot be represented by 32-bit");
}

TEST(Hdf5BooleanTest, SmallerType) {
    auto path = "TEST-boolean.h5";

    // Smaller types are allowed as long as they fit in the 32-bit signed integer.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "boolean");
        write_numbers<int>(vhandle, "data", { 1, 0, 1, 0 }, H5::PredType::NATIVE_UINT8);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::BOOLEAN);
    auto iptr = static_cast<const DefaultBooleanVector*>(parsed.get());
    EXPECT_EQ(iptr->base.values[0], 1);
    EXPECT_EQ(iptr->base.values[3], 0);
}

TEST(Hdf5BooleanTest, LegacyMissing) {
    auto path = "TEST-boolean.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "boolean");
        write_numbers<int>(vhandle, "data", { 1, 0, -2147483648, 0, 1 }, H5::PredType::NATIVE_INT32);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::BOOLEAN);
    auto bptr = static_cast<const DefaultBooleanVector*>(parsed.get());
    EXPECT_EQ(bptr->size(), 5);
    EXPECT_EQ(bptr->base.values[2], 255); // i.e., the test's missing value placeholder.
}

TEST(Hdf5BooleanTest, MissingPlaceholder) {
    auto path = "TEST-boolean.h5";

    // Except in the latest version.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "boolean");
        add_version(vhandle, "1.1");
        write_numbers<int>(vhandle, "data", { 1, 0, -2147483648, 0, 1 }, H5::PredType::NATIVE_INT32);
    }
    expect_hdf5_error(path, "blub", "boolean values should be");

    // Unless we specify a placeholder.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "boolean");
        add_version(vhandle, "1.1");
        auto dhandle = write_numbers<int>(vhandle, "data", { 1, 0, 2, 0, 1 }, H5::PredType::NATIVE_UINT8);
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT32, H5S_SCALAR);
        int placeholder = 2;
        ahandle.write(H5::PredType::NATIVE_INT, &placeholder);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::BOOLEAN);
        auto bptr = static_cast<const DefaultBooleanVector*>(parsed.get());
        EXPECT_EQ(bptr->size(), 5);
        EXPECT_EQ(bptr->base.values[2], 255); // i.e., the test's missing value placeholder.
    }
}

TEST(Hdf5BooleanTest, MissingPlaceholderError) {
    auto path = "TEST-integer.h5";

    // Format version < 1.2 only needs the same type class.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "boolean");
        add_version(ghandle, "1.1");
        auto dhandle = write_numbers<double>(ghandle, "data", { 0, 1, 0, 0, 1 }, H5::PredType::NATIVE_INT32);
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR);
    }
    expect_hdf5_error(path, "foo", "same type class as");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "boolean");
        add_version(ghandle, "1.1");
        auto dhandle = write_numbers<double>(ghandle, "data", { 0, 1, 0, 0, 1 }, H5::PredType::NATIVE_INT32);
        constexpr hsize_t one = 1;
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT32, H5::DataSpace(1, &one));
    }
    expect_hdf5_error(path, "foo", "scalar");

    // Format version >= 1.2 requires the exact same datatype.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "boolean");
        add_version(ghandle, "1.2");
        auto dhandle = write_numbers<double>(ghandle, "data", { 0, 1, 0, 0, 1 }, H5::PredType::NATIVE_UINT8);
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT8, H5S_SCALAR);
    }
    expect_hdf5_error(path, "foo", "same type as");
}

TEST(Hdf5BooleanTest, OutOfRange) {
    auto path = "TEST-boolean.h5";

    std::vector<int> collected(25000);
    for (size_t i = 0; i < collected.size(); ++i) {
        collected[i] = i % 2;
    }

    // Injecting invalid values at the front and the back, to confirm that we test all entries for validity.
    {
        collected.front() = 100;
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "boolean");
        write_numbers(ghandle, "data", collected, H5::PredType::NATIVE_INT32, /* chunk_size = */ 1239);
        collected.front() = 0;
    }
    expect_hdf5_error(path, "foo", "boolean values should be");

    {
        collected.back() = 100;
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "boolean");
        write_numbers(ghandle, "data", collected, H5::PredType::NATIVE_INT32, /* chunk_size = */ 1239);
        collected.back() = 0;
    }
    expect_hdf5_error(path, "foo", "boolean values should be");
}

TEST(JsonBooleanTest, SimpleLoading) {
    auto parsed = load_json("{ \"type\": \"boolean\", \"values\": [ true, false, false, true ] }");
    EXPECT_EQ(parsed->type(), uzuki2::BOOLEAN);
    auto bptr = static_cast<const DefaultBooleanVector*>(parsed.get());
    EXPECT_EQ(bptr->size(), 4);
    EXPECT_FALSE(bptr->base.scalar);
    EXPECT_EQ(bptr->base.values[0], 1);
    EXPECT_EQ(bptr->base.values[1], 0);

    // Works with scalars.
    {
        auto parsed = load_json("{ \"type\": \"boolean\", \"values\": true }");
        EXPECT_EQ(parsed->type(), uzuki2::BOOLEAN);
        auto stuff = static_cast<const DefaultBooleanVector*>(parsed.get());
        EXPECT_TRUE(stuff->base.scalar);
        EXPECT_TRUE(stuff->base.values[0]);
    }

    /********************************************
     *** See integer.cpp for tests for names. ***
     ********************************************/
}

TEST(JsonBooleanTest, MissingValues) {
    auto parsed = load_json("{ \"type\": \"boolean\", \"values\": [ true, null ] }");
    EXPECT_EQ(parsed->type(), uzuki2::BOOLEAN);
    auto bptr = static_cast<const DefaultBooleanVector*>(parsed.get());
    EXPECT_EQ(bptr->size(), 2);
    EXPECT_EQ(bptr->base.values.back(), 255);
}

TEST(JsonBooleanTest, CheckError) {
    expect_json_error("{\"type\":\"boolean\", \"values\":[true,1,2] }", "expected a boolean");

    /***********************************************
     *** See integer.cpp for vector error tests. ***
     ***********************************************/
}
