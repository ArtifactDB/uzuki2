#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_hdf5.hpp"

#include "test_subclass.h"
#include "utils.h"

TEST(Hdf5FactorTest, SimpleLoading) {
    auto path = "TEST-factor.h5";

    // Simple stuff works correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        create_dataset<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT);
        create_dataset(vhandle, "levels", { "Albo", "Rudd", "Gillard" });
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
        auto fptr = static_cast<const DefaultFactor*>(parsed.get());
        EXPECT_EQ(fptr->size(), 5);
        EXPECT_EQ(fptr->vbase.values.front(), 0);
        EXPECT_EQ(fptr->vbase.values.back(), 1);

        EXPECT_EQ(fptr->levels[0], "Albo");
        EXPECT_EQ(fptr->levels[2], "Gillard");
    }

    /********************************************
     *** See integer.cpp for tests for names. ***
     ********************************************/
}

TEST(Hdf5FactorTest, OrderedLoading) {
    auto path = "TEST-factor.h5";

    // Simple stuff works correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "ordered");
        create_dataset<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT);
        create_dataset(vhandle, "levels", { "Albo", "Rudd", "Gillard" });
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
        auto fptr = static_cast<const DefaultFactor*>(parsed.get());
        EXPECT_TRUE(fptr->ordered);
    }
}

TEST(Hdf5FactorTest, MissingValues) {
    auto path = "TEST-factor.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        create_dataset<int>(vhandle, "data", { 1, 2, -2147483648, 0, -2147483648 }, H5::PredType::NATIVE_INT);
        create_dataset(vhandle, "levels", { "Turnbull", "Morrison", "Abbott" });
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
        auto fptr = static_cast<const DefaultFactor*>(parsed.get());
        EXPECT_EQ(fptr->size(), 5);
        EXPECT_EQ(fptr->vbase.values[2], -1); // i.e., the test's missing value placeholder.
        EXPECT_EQ(fptr->vbase.values[4], -1); 
    }
}

TEST(Hdf5FactorTest, CheckError) {
    auto path = "TEST-factor.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        create_dataset<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT);
        create_dataset(vhandle, "levels", { "Albo", "Rudd" });
    }
    expect_hdf5_error(path, "blub", "less than the number of levels");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        create_dataset<int>(vhandle, "data", { 0, 1, -1, -1, 1 }, H5::PredType::NATIVE_INT);
        create_dataset(vhandle, "levels", { "Albo", "Rudd" });
    }
    expect_hdf5_error(path, "blub", "non-negative");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        create_dataset<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT);
        create_dataset(vhandle, "levels", { "Malcolm", "Malcolm", "John" });
    }
    expect_hdf5_error(path, "blub", "unique");

    /***********************************************
     *** See integer.cpp for vector error tests. ***
     ***********************************************/
}

TEST(JsonFactorTest, SimpleLoading) {
    auto parsed = load_json("{ \"type\": \"factor\", \"values\": [ 0, 1, 1, 0, 2 ], \"levels\": [ \"akari\", \"alice\", \"aika\" ] }");
    EXPECT_EQ(parsed->type(), uzuki2::FACTOR);

    auto fptr = static_cast<const DefaultFactor*>(parsed.get());
    EXPECT_EQ(fptr->size(), 5);
    EXPECT_EQ(fptr->vbase.values.front(), 0);
    EXPECT_EQ(fptr->vbase.values.back(), 2);

    EXPECT_EQ(fptr->levels[0], "akari");
    EXPECT_EQ(fptr->levels[2], "aika");

    /********************************************
     *** See integer.cpp for tests for names. ***
     ********************************************/
}

TEST(JsonFactorTest, OrderedLoading_v1_1) {
    {
        auto parsed = load_json("{ \"type\": \"factor\", \"values\": [ 2, 1, 0 ], \"levels\": [ \"athena\", \"akira\", \"alicia\" ], \"ordered\": true, \"version\": \"1.1\" }");
        EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
        auto fptr = static_cast<const DefaultFactor*>(parsed.get());
        EXPECT_TRUE(fptr->ordered);
    }

    {
        auto parsed = load_json("{ \"type\": \"factor\", \"values\": [ 2, 1, 0 ], \"levels\": [ \"athena\", \"akira\", \"alicia\" ], \"ordered\": false, \"version\": \"1.1\" }");
        EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
        auto fptr = static_cast<const DefaultFactor*>(parsed.get());
        EXPECT_FALSE(fptr->ordered);
    }

    expect_json_error("{ \"type\": \"factor\", \"values\": [ 1, 0 ], \"levels\": [ \"athena\", \"akira\", \"alicia\" ], \"ordered\": 1, \"version\": \"1.1\" }", "expected a boolean");
    expect_json_error("{ \"type\": \"ordered\", \"values\": [ 1, 0 ], \"levels\": [ \"athena\", \"akira\", \"alicia\" ], \"version\": \"1.1\" }", "unknown object type 'ordered'");
}


TEST(JsonFactorTest, OrderedLoading_v_1_0) {
    auto parsed = load_json("{ \"type\": \"ordered\", \"values\": [ 2, 1, 0 ], \"levels\": [ \"athena\", \"akira\", \"alicia\" ] }");
    EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
    auto fptr = static_cast<const DefaultFactor*>(parsed.get());
    EXPECT_TRUE(fptr->ordered);
}

TEST(JsonFactorTest, MissingValues) {
    auto parsed = load_json("{ \"type\": \"ordered\", \"values\": [ 2, 1, null, 0, null ], \"levels\": [ \"athena\", \"akira\", \"alicia\" ] }");
    EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
    auto fptr = static_cast<const DefaultFactor*>(parsed.get());
    EXPECT_EQ(fptr->vbase.values[2], -1); // i.e., the test's missing value placeholder.
    EXPECT_EQ(fptr->vbase.values[4], -1); 
}

TEST(JsonFactorTest, CheckError) {
    expect_json_error("{ \"type\": \"ordered\", \"values\": [ true, 0 ], \"levels\": [ \"athena\", \"akira\", \"alicia\" ] }", "expected a number");
    expect_json_error("{ \"type\": \"ordered\", \"values\": [ 1.2, 0 ], \"levels\": [ \"athena\", \"akira\", \"alicia\" ] }", "expected an integer");

    expect_json_error("{ \"type\": \"ordered\", \"values\": [ 2, 1, 3, 0 ], \"levels\": [ \"athena\", \"akira\", \"alicia\" ] }", "out of range");
    expect_json_error("{ \"type\": \"ordered\", \"values\": [ 2, 1, -1, 0 ], \"levels\": [ \"athena\", \"akira\", \"alicia\" ] }", "out of range");
    expect_json_error("{ \"type\": \"ordered\", \"values\": [ 2, 1, 0 ], \"levels\": [ \"aria\", \"aria\", \"aria\" ] }", "duplicate string");

    /***********************************************
     *** See integer.cpp for vector error tests. ***
     ***********************************************/
}
