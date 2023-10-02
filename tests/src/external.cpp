#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_hdf5.hpp"

#include "test_subclass.h"
#include "utils.h"

TEST(Hdf5ExternalTest, SimpleLoading) {
    auto path = "TEST-external.h5";

    // Simple stuff works correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = external_opener(handle, "foo");
        write_scalar(ghandle, "index", 0, H5::PredType::NATIVE_INT);
    }
    {
        DefaultExternals ext(1);
        auto parsed = uzuki2::Hdf5Parser().parse<DefaultProvisioner>(path, "foo", ext);
        EXPECT_EQ(parsed->type(), uzuki2::EXTERNAL);

        auto stuff = static_cast<const DefaultExternal*>(parsed.get());
        EXPECT_EQ(reinterpret_cast<uintptr_t>(stuff->ptr), 1);
    }

    // Multiple entries.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = list_opener(handle, "foo");
        auto dhandle = ghandle.createGroup("data");
        auto ohandle1 = external_opener(dhandle, "0");
        write_scalar(ohandle1, "index", 1, H5::PredType::NATIVE_INT);
        auto ohandle2 = external_opener(dhandle, "1");
        write_scalar(ohandle2, "index", 0, H5::PredType::NATIVE_INT);
    }
    {
        DefaultExternals ext(2);
        auto parsed = uzuki2::Hdf5Parser().parse<DefaultProvisioner>(path, "foo", ext);
        EXPECT_EQ(parsed->type(), uzuki2::LIST);
        auto list = static_cast<const DefaultList*>(parsed.get());

        auto stuff = static_cast<const DefaultExternal*>(list->values[0].get());
        EXPECT_EQ(reinterpret_cast<uintptr_t>(stuff->ptr), 2);

        auto stuff2 = static_cast<const DefaultExternal*>(list->values[1].get());
        EXPECT_EQ(reinterpret_cast<uintptr_t>(stuff2->ptr), 1);
    }
}

void expect_hdf5_external_error(std::string path, std::string name, std::string msg, int num_expected) {
    H5::H5File file(path, H5F_ACC_RDONLY); 
    EXPECT_ANY_THROW({
        try {
            uzuki2::Hdf5Parser().validate(file.openGroup(name), name, num_expected);
        } catch (std::exception& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
            throw;
        }
    });
}

TEST(Hdf5ExternalTest, CheckErrors) {
    auto path = "TEST-external.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = external_opener(handle, "foo");
        create_dataset<int>(ghandle, "index", { 0, 1 }, H5::PredType::NATIVE_INT);
    }
    expect_hdf5_external_error(path, "foo", "expected scalar", 1);

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = external_opener(handle, "foo");
        write_scalar(ghandle, "index", 0, H5::PredType::NATIVE_DOUBLE);
    }
    expect_hdf5_external_error(path, "foo", "expected integer", 1);

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = external_opener(handle, "foo");
        write_scalar(ghandle, "index", 1, H5::PredType::NATIVE_INT);
    }
    expect_hdf5_external_error(path, "foo", "out of range", 1);

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = external_opener(handle, "foo");
        write_scalar(ghandle, "index", 0, H5::PredType::NATIVE_INT);
    }
    expect_hdf5_external_error(path, "foo", "fewer instances", 2);

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = list_opener(handle, "foo");
        auto dhandle = ghandle.createGroup("data");
        auto ohandle1 = external_opener(dhandle, "0");
        write_scalar(ohandle1, "index", 0, H5::PredType::NATIVE_INT);
        auto ohandle2 = external_opener(dhandle, "1");
        write_scalar(ohandle2, "index", 0, H5::PredType::NATIVE_INT);
    }
    expect_hdf5_external_error(path, "foo", "consecutive", 2);
}

auto load_json_with_externals(std::string x, int num_externals) {
    DefaultExternals ext(num_externals);
    return uzuki2::JsonParser().parse_buffer<DefaultProvisioner>(reinterpret_cast<const unsigned char*>(x.c_str()), x.size(), ext);
}

TEST(JsonExternalTest, SimpleLoading) {
    // Single entry.
    {
        auto parsed = load_json_with_externals("{ \"type\": \"external\", \"index\": 0 }", 1);
        EXPECT_EQ(parsed->type(), uzuki2::EXTERNAL);
        auto stuff = static_cast<const DefaultExternal*>(parsed.get());
        EXPECT_EQ(reinterpret_cast<uintptr_t>(stuff->ptr), 1);
    }

    // Multiple entries.
    {
        auto parsed = load_json_with_externals("{ \"type\": \"list\", \"values\": [ { \"type\": \"external\", \"index\": 1 }, { \"type\": \"external\", \"index\": 0 } ] }", 2);
        EXPECT_EQ(parsed->type(), uzuki2::LIST);
        auto list = static_cast<const DefaultList*>(parsed.get());

        auto stuff = static_cast<const DefaultExternal*>(list->values[0].get());
        EXPECT_EQ(reinterpret_cast<uintptr_t>(stuff->ptr), 2);

        auto stuff2 = static_cast<const DefaultExternal*>(list->values[1].get());
        EXPECT_EQ(reinterpret_cast<uintptr_t>(stuff2->ptr), 1);
    }
}

void expect_json_external_error(std::string x, std::string msg, int num_expected) {
    EXPECT_ANY_THROW({
        try {
            uzuki2::JsonParser().validate_buffer(reinterpret_cast<const unsigned char*>(x.c_str()), x.size(), num_expected);
        } catch (std::exception& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
            throw;
        }
    });
}

TEST(JsonExternalTest, CheckErrors) {
    expect_json_external_error("{\"type\":\"external\"}", "expected 'index'", 1);
    expect_json_external_error("{\"type\":\"external\", \"index\":false}", "expected a number", 1);
    expect_json_external_error("{\"type\":\"external\", \"index\":1.2}", "expected an integer", 1);
    expect_json_external_error("{\"type\":\"external\", \"index\":-1}", "out of range", 1);
    expect_json_external_error("{ \"type\": \"list\", \"values\": [ { \"type\": \"external\", \"index\": 0 }, { \"type\": \"external\", \"index\": 0 } ] }", "consecutive", 2);
}
