#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_hdf5.hpp"

#include "ritsuko/ritsuko.hpp"

#include "test_subclass.h"
#include "utils.h"

TEST(Hdf5NumberTest, Vector) {
    auto path = "TEST-number.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "number");
        write_numbers<double>(vhandle, "data", { -1, 2, 3, 4 }, H5::PredType::NATIVE_DOUBLE);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::NUMBER);
    auto bptr = static_cast<const DefaultNumberVector*>(parsed.get());
    EXPECT_EQ(bptr->size(), 4);
    EXPECT_EQ(bptr->base.values.front(), -1);
    EXPECT_EQ(bptr->base.values.back(), 4);
    EXPECT_FALSE(bptr->base.scalar);
}

TEST(Hdf5NumberTest, Scalar) {
    auto path = "TEST-number.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "number");
        write_number(vhandle, "data", -1234.567, H5::PredType::NATIVE_DOUBLE);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::NUMBER);
    auto bptr = static_cast<const DefaultNumberVector*>(parsed.get());
    EXPECT_EQ(bptr->size(), 1);
    EXPECT_EQ(bptr->base.values.front(), -1234.567);
    EXPECT_TRUE(bptr->base.scalar);
}

TEST(Hdf5NumberTest, ChunkStream) {
    auto path = "TEST-string.h5";

    // Simulate multiple chunks so that we test the while{} loop for streaming values.
    const std::size_t len = 25000;
    std::vector<double> collected(len);
    for (std::size_t i = 0; i < len; ++i) {
        collected[i] = i * 1.5; 
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "number");
        write_numbers<double>(vhandle, "data", collected, H5::PredType::NATIVE_DOUBLE, /* chunk_size = */ 1298);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::NUMBER);
    auto nptr = static_cast<const DefaultNumberVector*>(parsed.get());
    EXPECT_EQ(nptr->base.values, collected);
}

TEST(Hdf5NumberTest, ForbiddenTypes1_0) {
    auto path = "TEST-forbidden.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "number");
        write_numbers<int>(vhandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_UINT32);
    }
    expect_hdf5_error(path, "blub", "expected a floating-point dataset");
}

TEST(Hdf5NumberTest, ForbiddenTypes1_3) {
    auto path = "TEST-forbidden.h5";

    // An integer dataset is allowed, but not if the type is so large that it might not fit in a float. 
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "number");
        write_numbers<int>(vhandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT64);
        add_version(handle.openGroup("blub"), "1.3");
    }
    expect_hdf5_error(path, "blub", "cannot be represented by 64-bit");

    // Strings are obviously not allowed.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "number");
        write_strings(vhandle, "data", { "foo", "bar" });
        add_version(handle.openGroup("blub"), "1.3");
    }
    expect_hdf5_error(path, "blub", "cannot be represented by 64-bit");
}

TEST(Hdf5NumberTest, SmallerType) {
    auto path = "TEST-number.h5";

    // Smaller types are allowed as long as they fit in the 64-bit float.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "number");
        write_numbers<float>(vhandle, "data", { 0.5, -0.25, 1, -2 }, H5::PredType::NATIVE_FLOAT);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::NUMBER);
        auto iptr = static_cast<const DefaultNumberVector*>(parsed.get());
        EXPECT_EQ(iptr->base.values[0], 0.5);
        EXPECT_EQ(iptr->base.values[3], -2);
    }

    // Format versions >= 1.3 can auto-cast a small integer dataset into a float.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "number");
        write_numbers<int>(vhandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_UINT32);
        add_version(handle.openGroup("blub"), "1.3");
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::NUMBER);
        auto iptr = static_cast<const DefaultNumberVector*>(parsed.get());
        EXPECT_EQ(iptr->base.values[0], 1);
        EXPECT_EQ(iptr->base.values[4], 5);
    }
}

TEST(Hdf5NumberTest, Missing1_0) {
    auto path = "TEST-number.h5";

    auto missing = uzuki2::hdf5::r_missing_value();
    auto nan = std::numeric_limits<double>::quiet_NaN();
    EXPECT_TRUE(std::isnan(missing));

    // Format 1.0 used the missing R value.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "number");
        write_numbers<double>(vhandle, "data", { 1, 0, missing, 0, nan, 1 }, H5::PredType::NATIVE_DOUBLE);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::NUMBER);
        auto bptr = static_cast<const DefaultNumberVector*>(parsed.get());
        EXPECT_EQ(bptr->size(), 6);
        EXPECT_EQ(bptr->base.values[2], -123456789);
        EXPECT_TRUE(std::isnan(bptr->base.values[4]));
    }
}

TEST(Hdf5NumberTest, Missing1_1) {
    auto path = "TEST-number.h5";

    auto missing = uzuki2::hdf5::r_missing_value();
    auto nan = std::numeric_limits<double>::quiet_NaN();

    // The missing R payload is no longer directly supported in format versions >= 1.1.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "number");
        add_version(vhandle, "1.1");
        write_numbers<double>(vhandle, "data", { 1, 0, missing, 0, nan, 1 }, H5::PredType::NATIVE_DOUBLE);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::NUMBER);
        auto bptr = static_cast<const DefaultNumberVector*>(parsed.get());
        EXPECT_EQ(bptr->size(), 6);
        EXPECT_TRUE(std::isnan(bptr->base.values[2]));
        EXPECT_TRUE(std::isnan(bptr->base.values[4]));
    }

    // Unless we specify it as the placeholder.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "number");
        add_version(vhandle, "1.1");

        auto dhandle = write_numbers<double>(vhandle, "data", { 1, 0, missing, 0, nan, 1 }, H5::PredType::NATIVE_DOUBLE);
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR);
        ahandle.write(H5::PredType::NATIVE_DOUBLE, &missing);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::NUMBER);
        auto bptr = static_cast<const DefaultNumberVector*>(parsed.get());
        EXPECT_EQ(bptr->size(), 6);
        EXPECT_EQ(bptr->base.values[2], -123456789);
        EXPECT_TRUE(std::isnan(bptr->base.values[4]));
    }
}

TEST(Hdf5NumberTest, Missing1_3) {
    auto path = "TEST-number.h5";

    auto missing = uzuki2::hdf5::r_missing_value();
    auto nan = std::numeric_limits<double>::quiet_NaN();

    // In version 1.3, the NaN payload is now ignored, as it's too fragile.
    // This means that all NaNs are considered to be missing if the placeholder is an NaN of any kind.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "number");
        add_version(vhandle, "1.3");

        auto dhandle = write_numbers<double>(vhandle, "data", { 1, 0, missing, 0, nan, 1 }, H5::PredType::NATIVE_DOUBLE);
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR);
        ahandle.write(H5::PredType::NATIVE_DOUBLE, &missing);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::NUMBER);
        auto bptr = static_cast<const DefaultNumberVector*>(parsed.get());
        EXPECT_EQ(bptr->size(), 6);
        EXPECT_EQ(bptr->base.values[2], -123456789);
        EXPECT_EQ(bptr->base.values[4], -123456789);
    }

    // Of course, non-NaN placeholders are still properly respected.
    auto inf = std::numeric_limits<double>::infinity();
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "number");
        add_version(vhandle, "1.3");

        auto dhandle = write_numbers<double>(vhandle, "data", { 1, 0, inf, 0, nan, 1 }, H5::PredType::NATIVE_DOUBLE);
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR);
        ahandle.write(H5::PredType::NATIVE_DOUBLE, &inf);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::NUMBER);
        auto bptr = static_cast<const DefaultNumberVector*>(parsed.get());
        EXPECT_EQ(bptr->size(), 6);
        EXPECT_EQ(bptr->base.values[2], -123456789);
        EXPECT_TRUE(std::isnan(bptr->base.values[4]));
    }
}

TEST(Hdf5NumberTest, MissingPlaceholderError) {
    auto path = "TEST-number.h5";

    // Format version < 1.2 only needs the same type class.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "number");
        add_version(ghandle, "1.1");
        auto dhandle = write_numbers<double>(ghandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_DOUBLE);
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    expect_hdf5_error(path, "foo", "same type class as");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "number");
        add_version(ghandle, "1.1");
        auto dhandle = write_numbers<double>(ghandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_DOUBLE);
        constexpr hsize_t one = 1;
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_DOUBLE, H5::DataSpace(1, &one));
    }
    expect_hdf5_error(path, "foo", "scalar");

    // Format version >= 1.2 requires the exact same datatype.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "number");
        auto dhandle = write_numbers<double>(ghandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_DOUBLE);
        add_version(ghandle, "1.2");
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_FLOAT, H5S_SCALAR);
    }
    expect_hdf5_error(path, "foo", "same type as");
}

TEST(JsonNumberTest, SimpleLoading) {
    auto parsed = load_json("{\"type\":\"number\", \"values\":[1.2, -3.5, -0.2, 1.343e+2] }");
    EXPECT_EQ(parsed->type(), uzuki2::NUMBER);
    auto bptr = static_cast<const DefaultNumberVector*>(parsed.get());
    EXPECT_EQ(bptr->size(), 4);
    EXPECT_FALSE(bptr->base.scalar);
    EXPECT_EQ(bptr->base.values.front(), 1.2);
    EXPECT_EQ(bptr->base.values.back(), 134.3);

    // Works with scalars.
    {
        auto parsed = load_json("{ \"type\": \"number\", \"values\": 12.34 }");
        EXPECT_EQ(parsed->type(), uzuki2::NUMBER);
        auto stuff = static_cast<const DefaultNumberVector*>(parsed.get());
        EXPECT_TRUE(stuff->base.scalar);
        EXPECT_EQ(stuff->base.values[0], 12.34);
    }

    /********************************************
     *** See integer.cpp for tests for names. ***
     ********************************************/
}

TEST(JsonNumberTest, MissingValues) {
    auto parsed = load_json("{\"type\":\"number\", \"values\":[1.2, null, \"Inf\", \"-Inf\", \"NaN\"] }");
    EXPECT_EQ(parsed->type(), uzuki2::NUMBER);

    auto bptr = static_cast<const DefaultNumberVector*>(parsed.get());
    EXPECT_EQ(bptr->size(), 5);
    EXPECT_EQ(bptr->base.values[1], -123456789);
    EXPECT_TRUE(std::isinf(bptr->base.values[2]));
    EXPECT_TRUE(bptr->base.values[2] > 0);
    EXPECT_TRUE(std::isinf(bptr->base.values[3]));
    EXPECT_TRUE(bptr->base.values[3] < 0);
    EXPECT_TRUE(std::isnan(bptr->base.values[4]));
}

TEST(JsonNumberTest, CheckError) {
    expect_json_error("{ \"type\": \"number\", \"values\": [true]}", "expected a number");
    expect_json_error("{ \"type\": \"number\", \"values\": [\"nan\"]}", "unsupported string");
}
