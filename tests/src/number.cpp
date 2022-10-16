#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse.hpp"

#include "test_subclass.h"
#include "utils.h"
#include "error.h"

TEST(NumberTest, SimpleLoading) {
    auto path = "TEST-number.h5";

    // Simple stuff works correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "number");
        create_dataset<double>(vhandle, "data", { -1, 2, 3, 4 }, H5::PredType::NATIVE_DOUBLE);
    }
    {
        auto parsed = uzuki2::parse<DefaultProvisioner>(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::NUMBER);
        auto bptr = static_cast<const DefaultNumberVector*>(parsed.get());
        EXPECT_EQ(bptr->size(), 4);
        EXPECT_EQ(bptr->base.values.front(), -1);
        EXPECT_EQ(bptr->base.values.back(), 4);
    }

    /********************************************
     *** See integer.cpp for tests for names. ***
     ********************************************/
}

TEST(NumberTest, MissingValues) {
    auto path = "TEST-number.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "number");
        create_dataset<double>(vhandle, "data", { 1, 0, std::numeric_limits<double>::quiet_NaN(), 0, 1 }, H5::PredType::NATIVE_DOUBLE);
    }
    {
        auto parsed = uzuki2::parse<DefaultProvisioner>(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::NUMBER);
        auto bptr = static_cast<const DefaultNumberVector*>(parsed.get());
        EXPECT_EQ(bptr->size(), 5);
        EXPECT_TRUE(std::isnan(bptr->base.values[2]));
    }
}

TEST(NumberTest, CheckError) {
    auto path = "TEST-number.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "number");
        create_dataset<int>(ghandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT);
    }
    expect_error(path, "foo", "expected a float");

    /***********************************************
     *** See integer.cpp for vector error tests. ***
     ***********************************************/
}
