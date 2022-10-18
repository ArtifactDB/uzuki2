#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_hdf5.hpp"

#include "test_subclass.h"
#include "utils.h"

TEST(Hdf5BooleanTest, SimpleLoading) {
    auto path = "TEST-boolean.h5";

    // Simple stuff works correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "boolean");
        create_dataset<int>(vhandle, "data", { 1, 0, 1, 0, 0 }, H5::PredType::NATIVE_INT);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::BOOLEAN);
        auto bptr = static_cast<const DefaultBooleanVector*>(parsed.get());
        EXPECT_EQ(bptr->size(), 5);
        EXPECT_EQ(bptr->base.values.front(), 1);
        EXPECT_EQ(bptr->base.values.back(), 0);
    }

    /********************************************
     *** See integer.cpp for tests for names. ***
     ********************************************/
}

TEST(Hdf5BooleanTest, MissingValues) {
    auto path = "TEST-boolean.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "boolean");
        create_dataset<int>(vhandle, "data", { 1, 0, -2147483648, 0, 1 }, H5::PredType::NATIVE_INT);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::BOOLEAN);
        auto bptr = static_cast<const DefaultBooleanVector*>(parsed.get());
        EXPECT_EQ(bptr->size(), 5);
        EXPECT_EQ(bptr->base.values[2], 255); // i.e., the test's missing value placeholder.
    }
}

TEST(Hdf5BooleanTest, CheckError) {
    auto path = "TEST-boolean.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "boolean");
        create_dataset<double>(ghandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT);
    }
    expect_hdf5_error(path, "foo", "boolean values should be");

    /***********************************************
     *** See integer.cpp for vector error tests. ***
     ***********************************************/
}

TEST(JsonBooleanTest, SimpleLoading) {
    auto parsed = load_json("{ \"type\": \"boolean\", \"values\": [ true, false, false, true ] }");
    EXPECT_EQ(parsed->type(), uzuki2::BOOLEAN);
    auto bptr = static_cast<const DefaultBooleanVector*>(parsed.get());
    EXPECT_EQ(bptr->size(), 4);
    EXPECT_EQ(bptr->base.values[0], 1);
    EXPECT_EQ(bptr->base.values[1], 0);

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
