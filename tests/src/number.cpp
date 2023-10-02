#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_hdf5.hpp"

#include "test_subclass.h"
#include "utils.h"

TEST(Hdf5NumberTest, SimpleLoading) {
    auto path = "TEST-number.h5";

    // Simple stuff works correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "number");
        create_dataset<double>(vhandle, "data", { -1, 2, 3, 4 }, H5::PredType::NATIVE_DOUBLE);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::NUMBER);
        auto bptr = static_cast<const DefaultNumberVector*>(parsed.get());
        EXPECT_EQ(bptr->size(), 4);
        EXPECT_EQ(bptr->base.values.front(), -1);
        EXPECT_EQ(bptr->base.values.back(), 4);
        EXPECT_FALSE(bptr->base.scalar);
    }

    // Scalars work correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "number");
        write_scalar(vhandle, "data", -1234.567, H5::PredType::NATIVE_DOUBLE);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::NUMBER);
        auto bptr = static_cast<const DefaultNumberVector*>(parsed.get());
        EXPECT_EQ(bptr->size(), 1);
        EXPECT_EQ(bptr->base.values.front(), -1234.567);
        EXPECT_TRUE(bptr->base.scalar);
    }

    /********************************************
     *** See integer.cpp for tests for names. ***
     ********************************************/
}

TEST(Hdf5NumberTest, MissingValues) {
    auto path = "TEST-number.h5";

    auto missing = uzuki2::hdf5::legacy_missing_double();
    EXPECT_TRUE(std::isnan(missing));

    // Old version used the missing R value.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "number");
        create_dataset<double>(vhandle, "data", { 1, 0, missing, 0, 1 }, H5::PredType::NATIVE_DOUBLE);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::NUMBER);
        auto bptr = static_cast<const DefaultNumberVector*>(parsed.get());
        EXPECT_EQ(bptr->size(), 5);
        EXPECT_EQ(bptr->base.values[2], -123456789);
    }

    // This is no longer directly supported in the new version.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "number");
        add_version(vhandle, "1.1");
        create_dataset<double>(vhandle, "data", { 1, 0, missing, 0, 1 }, H5::PredType::NATIVE_DOUBLE);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::NUMBER);
        auto bptr = static_cast<const DefaultNumberVector*>(parsed.get());
        EXPECT_EQ(bptr->size(), 5);
        EXPECT_TRUE(std::isnan(bptr->base.values[2]));
    }

    // Unless we specify it.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "number");
        add_version(vhandle, "1.1");

        auto dhandle = create_dataset<double>(vhandle, "data", { 1, 0, missing, 0, 1 }, H5::PredType::NATIVE_DOUBLE);
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR);
        ahandle.write(H5::PredType::NATIVE_DOUBLE, &missing);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::NUMBER);
        auto bptr = static_cast<const DefaultNumberVector*>(parsed.get());
        EXPECT_EQ(bptr->size(), 5);
        EXPECT_EQ(bptr->base.values[2], -123456789);
    }
}

TEST(Hdf5NumberTest, CheckError) {
    auto path = "TEST-number.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "number");
        create_dataset<int>(ghandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT);
    }
    expect_hdf5_error(path, "foo", "expected a float");

    /***********************************************
     *** See integer.cpp for vector error tests. ***
     ***********************************************/
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
