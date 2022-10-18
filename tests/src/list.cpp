#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse.hpp"

#include "test_subclass.h"
#include "utils.h"

TEST(Hdf5ListTest, SimpleLoading) {
    auto path = "TEST-list.h5";

    // Simple stuff works correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = list_opener(handle, "foo");
        auto dhandle = ghandle.createGroup("data");
        nothing_opener(dhandle, "0");
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

TEST(Hdf5ListTest, NestedLoading) {
    auto path = "TEST-list.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = list_opener(handle, "foo");
        auto dhandle = ghandle.createGroup("data");
        nothing_opener(dhandle, "0");

        auto lhandle = list_opener(dhandle, "1");
        auto dhandle2 = lhandle.createGroup("data");
        nothing_opener(dhandle2, "0");
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

TEST(Hdf5ListTest, CheckError) {
    auto path = "TEST-list.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = list_opener(handle, "foo");
        create_dataset<int>(ghandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT);
    }
    expect_hdf5_error(path, "foo", "expected a group at 'foo/data'");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = list_opener(handle, "foo");
        auto dhandle = ghandle.createGroup("data");
        nothing_opener(dhandle, "1");
    }
    expect_hdf5_error(path, "foo", "expected a group at 'foo/data/0'");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = list_opener(handle, "foo");
        auto dhandle = ghandle.createGroup("data");
        create_dataset<int>(dhandle, "0", { 1, 2, 3 }, H5::PredType::NATIVE_INT);
    }
    expect_hdf5_error(path, "foo", "expected a group at 'foo/data/0'");
}


TEST(JsonListTest, SimpleLoading) {
    // Simple stuff works correctly.
    {
        auto parsed = load_json("{ \"type\":\"list\", \"values\": [ { \"type\": \"nothing\" }, { \"type\": \"integer\", \"values\": [ 1, 2, 3 ] } ] }");
        EXPECT_EQ(parsed->type(), uzuki2::LIST);

        auto stuff = static_cast<const DefaultList*>(parsed.get());
        EXPECT_EQ(stuff->size(), 2);

        EXPECT_EQ(stuff->values[0]->type(), uzuki2::NOTHING);
        EXPECT_EQ(stuff->values[1]->type(), uzuki2::INTEGER);

        auto iptr = static_cast<const DefaultIntegerVector*>(stuff->values[1].get());
        EXPECT_EQ(iptr->size(), 3);
        EXPECT_EQ(iptr->base.values.front(), 1);
        EXPECT_EQ(iptr->base.values.back(), 3);
    }

    // Works with names.
    {
        auto parsed = load_json("{ \"type\":\"list\", \"values\": [ { \"type\": \"nothing\" }, { \"type\": \"integer\", \"values\": [ 1, 2, 3 ] } ], \"names\": [ \"X\", \"Y\" ] }");
        EXPECT_EQ(parsed->type(), uzuki2::LIST);

        auto stuff = static_cast<const DefaultList*>(parsed.get());
        EXPECT_TRUE(stuff->has_names);
        EXPECT_EQ(stuff->names[0], "X");
        EXPECT_EQ(stuff->names[1], "Y");
    }

    // Works if empty.
    {
        auto parsed = load_json("{ \"type\":\"list\", \"values\": [] }");
        EXPECT_EQ(parsed->type(), uzuki2::LIST);
        auto stuff = static_cast<const DefaultList*>(parsed.get());
        EXPECT_EQ(stuff->size(), 0);
    }
}

TEST(JsonListTest, NestedLoading) {
    auto parsed = load_json("{ \"type\":\"list\", \"values\": [ { \"type\": \"nothing\" }, { \"type\": \"list\", \"values\": [ { \"type\": \"nothing\" } ] } ] }");
    EXPECT_EQ(parsed->type(), uzuki2::LIST);

    auto stuff = static_cast<const DefaultList*>(parsed.get());
    EXPECT_EQ(stuff->size(), 2);

    EXPECT_EQ(stuff->values[0]->type(), uzuki2::NOTHING);
    EXPECT_EQ(stuff->values[1]->type(), uzuki2::LIST);

    auto lptr = static_cast<const DefaultList*>(stuff->values[1].get());
    EXPECT_EQ(lptr->size(), 1);
    EXPECT_EQ(lptr->values[0]->type(), uzuki2::NOTHING);
}

TEST(JsonListTest, CheckError) {
    expect_json_error("{ \"type\":\"list\" }", "expected 'values' property");
    expect_json_error("{ \"type\":\"list\", \"values\": 1 }", "expected an array");
    expect_json_error("{ \"type\":\"list\", \"values\": [true] }", "should be represented by a JSON object");
    expect_json_error("{ \"type\":\"list\", \"values\": [ { \"type\": \"nothing\" } ], \"names\": [\"X\", \"Y\"] }", "should be the same");
}
