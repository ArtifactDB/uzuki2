#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_json.hpp"

#include "utils.h"
#include "../test_subclass.h"

TEST(Hdf5External, Single) {
    auto path = "TEST-external.h5";
    uzuki2::hdf5::Options opt;
    opt.strict_list = false;

    // Simple stuff works correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = external_opener(handle, "foo");
        write_number(ghandle, "index", 0, H5::PredType::NATIVE_INT32);
    }

    DefaultExternals ext(1);
    auto parsed = uzuki2::hdf5::parse<DefaultProvisioner>(path, "foo", ext, opt);
    EXPECT_EQ(parsed->type(), uzuki2::EXTERNAL);

    auto stuff = static_cast<const DefaultExternal*>(parsed.get());
    EXPECT_EQ(reinterpret_cast<uintptr_t>(stuff->ptr), 1);
}

TEST(Hdf5External, Multiple) {
    auto path = "TEST-external.h5";
    uzuki2::hdf5::Options opt;
    opt.strict_list = false;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = list_opener(handle, "foo");
        auto dhandle = ghandle.createGroup("data");
        auto ohandle1 = external_opener(dhandle, "0");
        write_number(ohandle1, "index", 1, H5::PredType::NATIVE_INT32);
        auto ohandle2 = external_opener(dhandle, "1");
        write_number(ohandle2, "index", 0, H5::PredType::NATIVE_INT32);
    }

    DefaultExternals ext(2);
    auto parsed = uzuki2::hdf5::parse<DefaultProvisioner>(path, "foo", ext, opt);
    EXPECT_EQ(parsed->type(), uzuki2::LIST);
    auto list = static_cast<const DefaultList*>(parsed.get());

    auto stuff = static_cast<const DefaultExternal*>(list->values[0].get());
    EXPECT_EQ(reinterpret_cast<uintptr_t>(stuff->ptr), 2);

    auto stuff2 = static_cast<const DefaultExternal*>(list->values[1].get());
    EXPECT_EQ(reinterpret_cast<uintptr_t>(stuff2->ptr), 1);
}

void expect_hdf5_external_error(std::string path, std::string name, std::string msg, int num_expected) {
    H5::H5File file(path, H5F_ACC_RDONLY); 
    uzuki2::hdf5::Options opt;
    opt.strict_list = false;
    EXPECT_ANY_THROW({
        try {
            uzuki2::hdf5::validate(file.openGroup(name), num_expected, std::move(opt));
        } catch (std::exception& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
            throw;
        }
    });
}

TEST(Hdf5External, CheckErrors) {
    auto path = "TEST-external.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = external_opener(handle, "foo");
        write_numbers<std::int32_t>(ghandle, "index", { 0, 1 }, H5::PredType::NATIVE_INT32);
    }
    expect_hdf5_external_error(path, "foo", "expected scalar", 1);

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = external_opener(handle, "foo");
        write_number(ghandle, "index", 0, H5::PredType::NATIVE_DOUBLE);
    }
    expect_hdf5_external_error(path, "foo", "external index at 'index' cannot be represented", 1);

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = external_opener(handle, "foo");
        write_number(ghandle, "index", 1, H5::PredType::NATIVE_INT32);
    }
    expect_hdf5_external_error(path, "foo", "out of range", 1);

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = external_opener(handle, "foo");
        write_number(ghandle, "index", 0, H5::PredType::NATIVE_INT32);
    }
    expect_hdf5_external_error(path, "foo", "fewer instances", 2);

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = list_opener(handle, "foo");
        auto dhandle = ghandle.createGroup("data");
        auto ohandle1 = external_opener(dhandle, "0");
        write_number(ohandle1, "index", 0, H5::PredType::NATIVE_INT32);
        auto ohandle2 = external_opener(dhandle, "1");
        write_number(ohandle2, "index", 0, H5::PredType::NATIVE_INT32);
    }
    expect_hdf5_external_error(path, "foo", "consecutive", 2);
}
