#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <fstream>

#include "uzuki2/parse_hdf5.hpp"

#include "test_subclass.h"
#include "utils.h"

TEST(Hdf5AttributeTest, CheckError) {
    auto path = "TEST-date.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        handle.createGroup("whee");
    }
    expect_hdf5_error(path, "whee", "uzuki_object");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        std::map<std::string, std::string> attrs;
        attrs["uzuki_object"] = "blah";
        super_group_opener(handle, "whee", attrs);
    }
    expect_hdf5_error(path, "whee", "unknown uzuki2 object");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        std::map<std::string, std::string> attrs;
        attrs["uzuki_object"] = "vector";
        super_group_opener(handle, "whee", attrs);
    }
    expect_hdf5_error(path, "whee", "uzuki_type");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "whee", "BLAH");
        create_dataset<int>(vhandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT);
    }
    expect_hdf5_error(path, "whee", "unknown vector type");
}

class JsonFileTest : public ::testing::TestWithParam<int> {};

TEST_P(JsonFileTest, Chunking) {
    auto path = "TEST.json";
    size_t block_size = GetParam();

    // Checking we can make a read with different block types.
    {
        std::ofstream ohandle(path);
        ohandle << "{ \"type\":\"list\",\n  \"values\": [ { \"type\": \"nothing\" }, { \"type\": \"integer\", \"values\": [ 1, 2, 3 ] } ], \n\t\"names\": [ \"X\", \"Y\" ] }";
    }
    EXPECT_NO_THROW(uzuki2::validate_json(path));

    // Checking parsing is fine.
    {
        byteme::SomeFileReader reader(path, block_size);
        auto parsed = uzuki2::parse_json<DefaultProvisioner>(reader);
        EXPECT_EQ(parsed->type(), uzuki2::LIST);

        auto stuff = static_cast<const DefaultList*>(parsed.get());
        EXPECT_EQ(stuff->size(), 2);

        EXPECT_EQ(stuff->values[0]->type(), uzuki2::NOTHING);
        EXPECT_EQ(stuff->values[1]->type(), uzuki2::INTEGER);

        auto stuff2 = static_cast<const DefaultIntegerVector*>(stuff->values[1].get());
        EXPECT_EQ(stuff2->base.values[0], 1);
        EXPECT_EQ(stuff2->base.values[2], 3);

        EXPECT_TRUE(stuff->has_names);
        EXPECT_EQ(stuff->names[0], "X");
        EXPECT_EQ(stuff->names[1], "Y");
    }

    // Checking that the overload works with external references.
    {
        std::ofstream ohandle(path);
        ohandle << "{ \"type\":\"list\",\n  \"values\": [ { \"type\": \"external\", \"index\": 0 }, { \"type\": \"external\", \"index\": 1 } ] }";
    }
    EXPECT_NO_THROW(uzuki2::validate_json(path, 2));

    {
        byteme::SomeFileReader reader(path, block_size);
        DefaultExternals ext(2);
        auto parsed = uzuki2::parse_json<DefaultProvisioner>(reader, std::move(ext));
        EXPECT_EQ(parsed->type(), uzuki2::LIST);

        auto stuff = static_cast<const DefaultList*>(parsed.get());
        EXPECT_EQ(stuff->size(), 2);

        EXPECT_EQ(stuff->values[0]->type(), uzuki2::EXTERNAL);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(static_cast<const DefaultExternal*>(stuff->values[0].get())->ptr), 1);
        EXPECT_EQ(stuff->values[1]->type(), uzuki2::EXTERNAL);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(static_cast<const DefaultExternal*>(stuff->values[1].get())->ptr), 2);
    }
}

INSTANTIATE_TEST_SUITE_P(
    JsonFile,
    JsonFileTest,
    ::testing::Values(3, 7, 13, 17, 29)
);
