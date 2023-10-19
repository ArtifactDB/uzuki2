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

TEST(Hdf5IntegerTypeTest, Forbidden) {
    auto path = "TEST-forbidden.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        create_dataset<int>(vhandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_UINT32);
    }
    expect_hdf5_error(path, "blub", "potentially out of range");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        create_dataset<int>(vhandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT64);
    }
    expect_hdf5_error(path, "blub", "potentially out of range");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        create_dataset<int>(vhandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_UINT16);
    }
    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
    auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
    EXPECT_EQ(iptr->base.values[0], 1);
    EXPECT_EQ(iptr->base.values[4], 5);
}

class JsonFileTest : public ::testing::TestWithParam<std::tuple<int, bool> > {};

TEST_P(JsonFileTest, Chunking) {
    auto path = "TEST.json";
    auto param = GetParam();
    size_t block_size = std::get<0>(param);
    uzuki2::json::Options opt;
    opt.parallel = std::get<1>(param);

    {
        std::ofstream ohandle(path);
        ohandle << "{ \"type\":\"list\",\n  \"values\": [ { \"type\": \"nothing\" }, { \"type\": \"integer\", \"values\": [ 1, 2, 3 ] } ], \n\t\"names\": [ \"X\", \"Y\" ] }";
    }

    // Checking parsing is fine with different block types.
    {
        byteme::SomeFileReader reader(path, block_size);
        auto parsed = uzuki2::json::parse<DefaultProvisioner>(reader, opt);
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

    // Checking that validation works fine.
    {
        byteme::SomeFileReader reader(path, block_size);
        EXPECT_NO_THROW(uzuki2::json::validate(reader));
    }

    // Checking that the overload works with external references.
    {
        std::ofstream ohandle(path);
        ohandle << "{ \"type\":\"list\",\n  \"values\": [ { \"type\": \"external\", \"index\": 0 }, { \"type\": \"external\", \"index\": 1 } ] }";
    }

    {
        byteme::SomeFileReader reader(path, block_size);
        DefaultExternals ext(2);
        auto parsed = uzuki2::json::parse<DefaultProvisioner>(reader, std::move(ext));
        EXPECT_EQ(parsed->type(), uzuki2::LIST);

        auto stuff = static_cast<const DefaultList*>(parsed.get());
        EXPECT_EQ(stuff->size(), 2);

        EXPECT_EQ(stuff->values[0]->type(), uzuki2::EXTERNAL);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(static_cast<const DefaultExternal*>(stuff->values[0].get())->ptr), 1);
        EXPECT_EQ(stuff->values[1]->type(), uzuki2::EXTERNAL);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(static_cast<const DefaultExternal*>(stuff->values[1].get())->ptr), 2);
    }

    // Checking that validation works fine.
    {
        byteme::SomeFileReader reader(path, block_size);
        EXPECT_NO_THROW(uzuki2::json::validate(reader, 2));
    }
}

INSTANTIATE_TEST_SUITE_P(
    JsonFile,
    JsonFileTest,
    ::testing::Combine(
        ::testing::Values(3, 7, 13, 17, 29), // block size per read.
        ::testing::Values(false, true)  // whether or not it's parallelized.
    )
);

TEST(JsonFileTest, CheckMethods) {
    std::string path = "TEST.json";

    // Making sure the parse_file methods work correctly.
    {
        std::ofstream ohandle(path);
        ohandle << "{ \"type\":\"list\",\n  \"values\": [ { \"type\": \"nothing\" }, { \"type\": \"integer\", \"values\": [ 1, 2, 3 ] } ], \n\t\"names\": [ \"X\", \"Y\" ] }";
    }
    {
        auto parsed = uzuki2::json::parse_file<DefaultProvisioner>(path);
        EXPECT_EQ(parsed->type(), uzuki2::LIST);
        EXPECT_NO_THROW(uzuki2::json::validate_file(path));
    }

    // Again, with some externals.
    {
        std::ofstream ohandle(path);
        ohandle << "{ \"type\":\"list\",\n  \"values\": [ { \"type\": \"external\", \"index\": 0 }, { \"type\": \"external\", \"index\": 1 } ] }";
    }
    {
        auto parsed = uzuki2::json::parse_file<DefaultProvisioner>(path, DefaultExternals(2));
        EXPECT_EQ(parsed->type(), uzuki2::LIST);
        EXPECT_NO_THROW(uzuki2::json::validate_file(path, 2));
    }
}

TEST(JsonFileTest, CheckVersion) {
    expect_json_error("{ version: true }", "expected a string");
}

TEST(VersionParsing, Basic) {
    {
        auto v = uzuki2::parse_version_string("1.0");
        EXPECT_EQ(v.major, 1);
        EXPECT_EQ(v.minor, 0);
    }

    {
        auto v = uzuki2::parse_version_string("1.12");
        EXPECT_EQ(v.major, 1);
        EXPECT_EQ(v.minor, 12);
    }

    {
        auto v = uzuki2::parse_version_string("123.456");
        EXPECT_EQ(v.major, 123);
        EXPECT_EQ(v.minor, 456);
    }
}

static void expect_version_error(std::string version, std::string msg) {
    EXPECT_ANY_THROW({
        try {
            uzuki2::parse_version_string(version);
        } catch (std::exception& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
            throw;
        }
    });
}

TEST(VersionParsing, Errors) {
    expect_version_error("", "empty");
    expect_version_error("0.1", "leading zeros");
    expect_version_error("a.1", "non-digit");
    expect_version_error("1", "minor version");
    expect_version_error("1.", "minor version");
    expect_version_error("1.01", "leading zeros");
    expect_version_error("1.a", "non-digit");
}
