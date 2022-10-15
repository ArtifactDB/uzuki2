#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse.hpp"
#include "uzuki2/validate.hpp"
#include "test_subclass.h"
#include "utils.h"

TEST(ListTest, SimpleLoading) {
    auto path = "TEST-list.h5";

    // Simple stuff works correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = list_opener(handle, "foo");
        auto dhandle = ghandle.createGroup("data");
        null_opener(dhandle, "0");
        auto vhandle = vector_opener(dhandle, "1", "integer");
        create_dataset<int>(vhandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT);
    }
    {
        auto parsed = uzuki2::parse<DefaultProvisioner>(path, "foo");
        EXPECT_EQ(parsed->type(), uzuki2::LIST);

        auto stuff = static_cast<const DefaultList*>(parsed.get());
        EXPECT_EQ(stuff->size(), 2);

        EXPECT_EQ(stuff->values[0]->type(), uzuki2::NOTHING);
        EXPECT_EQ(stuff->values[1]->type(), uzuki2::INTEGER);

        auto iptr = static_cast<const DefaultIntegerVector*>(stuff->values[1].get());
        EXPECT_EQ(iptr->size(), 5);
        EXPECT_EQ(iptr->base.values.front(), 1);
        EXPECT_EQ(iptr->base.values.back(), 5);
    }

    // Works with names.
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("foo");
        create_dataset(ghandle, "names", { "bruce", "alfred" });
    }
    {
        auto parsed = uzuki2::parse<DefaultProvisioner>(path, "foo");
        EXPECT_EQ(parsed->type(), uzuki2::LIST);

        auto stuff = static_cast<const DefaultList*>(parsed.get());
        EXPECT_TRUE(stuff->has_names);
        EXPECT_EQ(stuff->names[0], "bruce");
        EXPECT_EQ(stuff->names[1], "alfred");
    }
}

TEST(ListTest, NestedLoading) {
    auto path = "TEST-list.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = list_opener(handle, "foo");
        auto dhandle = ghandle.createGroup("data");
        null_opener(dhandle, "0");

        auto lhandle = list_opener(dhandle, "1");
        auto dhandle2 = lhandle.createGroup("data");
        null_opener(dhandle2, "0");
    }

    {
        auto parsed = uzuki2::parse<DefaultProvisioner>(path, "foo");
        EXPECT_EQ(parsed->type(), uzuki2::LIST);

        auto stuff = static_cast<const DefaultList*>(parsed.get());
        EXPECT_EQ(stuff->size(), 2);

        EXPECT_EQ(stuff->values[0]->type(), uzuki2::NOTHING);
        EXPECT_EQ(stuff->values[1]->type(), uzuki2::LIST);

        auto lptr = static_cast<const DefaultList*>(stuff->values[1].get());
        EXPECT_EQ(lptr->size(), 1);
    }
}

static void expect_error(std::string file, std::string name, std::string msg) {
    EXPECT_ANY_THROW({
        try {
            uzuki2::validate(file, name);
        } catch (std::exception& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
            throw;
        }
    });
}

TEST(ListTest, CheckError) {
    auto path = "TEST-list.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = list_opener(handle, "foo");
        create_dataset<int>(ghandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT);
    }
    expect_error(path, "foo", "expected a group at 'foo/data'");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = list_opener(handle, "foo");
        auto dhandle = ghandle.createGroup("data");
        null_opener(dhandle, "1");
    }
    expect_error(path, "foo", "expected a group at 'foo/data/0'");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = list_opener(handle, "foo");
        auto dhandle = ghandle.createGroup("data");
        create_dataset<int>(dhandle, "0", { 1, 2, 3 }, H5::PredType::NATIVE_INT);
    }
    expect_error(path, "foo", "expected a group at 'foo/data/0'");
}

