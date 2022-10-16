#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse.hpp"

#include "test_subclass.h"
#include "utils.h"
#include "error.h"

TEST(FactorTest, SimpleLoading) {
    auto path = "TEST-factor.h5";

    // Simple stuff works correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        create_dataset<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT);
        create_dataset(vhandle, "levels", { "Albo", "Rudd", "Gillard" });
    }
    {
        auto parsed = uzuki2::parse<DefaultProvisioner>(path, "blub");
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

TEST(FactorTest, OrderedLoading) {
    auto path = "TEST-factor.h5";

    // Simple stuff works correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "ordered");
        create_dataset<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT);
        create_dataset(vhandle, "levels", { "Albo", "Rudd", "Gillard" });
    }
    {
        auto parsed = uzuki2::parse<DefaultProvisioner>(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
        auto fptr = static_cast<const DefaultFactor*>(parsed.get());
        EXPECT_TRUE(fptr->ordered);
    }
}

TEST(FactorTest, MissingValues) {
    auto path = "TEST-factor.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        create_dataset<int>(vhandle, "data", { 1, 2, -2147483648, 0, -2147483648 }, H5::PredType::NATIVE_INT);
        create_dataset(vhandle, "levels", { "Turnbull", "Morrison", "Abbott" });
    }
    {
        auto parsed = uzuki2::parse<DefaultProvisioner>(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
        auto fptr = static_cast<const DefaultFactor*>(parsed.get());
        EXPECT_EQ(fptr->size(), 5);
        EXPECT_EQ(fptr->vbase.values[2], -1); // i.e., the test's missing value placeholder.
        EXPECT_EQ(fptr->vbase.values[4], -1); 
    }
}

TEST(FactorTest, CheckError) {
    auto path = "TEST-factor.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        create_dataset<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT);
        create_dataset(vhandle, "levels", { "Albo", "Rudd" });
    }
    expect_error(path, "blub", "less than the number of levels");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        create_dataset<int>(vhandle, "data", { 0, 1, -1, -1, 1 }, H5::PredType::NATIVE_INT);
        create_dataset(vhandle, "levels", { "Albo", "Rudd" });
    }
    expect_error(path, "blub", "non-negative");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        create_dataset<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT);
        create_dataset(vhandle, "levels", { "Malcolm", "Malcolm", "John" });
    }
    expect_error(path, "blub", "unique");

    /***********************************************
     *** See integer.cpp for vector error tests. ***
     ***********************************************/
}
