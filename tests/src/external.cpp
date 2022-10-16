#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse.hpp"
#include "uzuki2/validate.hpp"

#include "test_subclass.h"
#include "utils.h"

TEST(OtherTest, SimpleLoading) {
    auto path = "TEST-other.h5";

    // Simple stuff works correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = other_opener(handle, "foo");
        write_scalar(ghandle, "index", 0, H5::PredType::NATIVE_INT);
    }
    {
        DefaultExternals ext(1);
        auto parsed = uzuki2::parse<DefaultProvisioner>(path, "foo", ext);
        EXPECT_EQ(parsed->type(), uzuki2::OTHER);

        auto stuff = static_cast<const DefaultOther*>(parsed.get());
        EXPECT_EQ(reinterpret_cast<uintptr_t>(stuff->ptr), 1);
    }

    // Multiple entries.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = list_opener(handle, "foo");
        auto dhandle = ghandle.createGroup("data");
        auto ohandle1 = other_opener(dhandle, "0");
        write_scalar(ohandle1, "index", 1, H5::PredType::NATIVE_INT);
        auto ohandle2 = other_opener(dhandle, "1");
        write_scalar(ohandle2, "index", 0, H5::PredType::NATIVE_INT);
    }
    {
        DefaultExternals ext(2);
        auto parsed = uzuki2::parse<DefaultProvisioner>(path, "foo", ext);
        EXPECT_EQ(parsed->type(), uzuki2::LIST);
        auto list = static_cast<const DefaultList*>(parsed.get());

        auto stuff = static_cast<const DefaultOther*>(list->values[0].get());
        EXPECT_EQ(reinterpret_cast<uintptr_t>(stuff->ptr), 2);

        auto stuff2 = static_cast<const DefaultOther*>(list->values[1].get());
        EXPECT_EQ(reinterpret_cast<uintptr_t>(stuff2->ptr), 1);
    }
}

static void expect_error(std::string path, std::string name, std::string msg, int num_expected) {
    H5::H5File file(path, H5F_ACC_RDONLY); 
    EXPECT_ANY_THROW({
        try {
            uzuki2::validate(file.openGroup(name), name, num_expected);
        } catch (std::exception& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
            throw;
        }
    });
}

TEST(OtherTest, CheckErrors) {
    auto path = "TEST-other.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = other_opener(handle, "foo");
        write_scalar(ghandle, "index", 1, H5::PredType::NATIVE_INT);
    }
    expect_error(path, "foo", "out of range", 1);

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = other_opener(handle, "foo");
        write_scalar(ghandle, "index", 0, H5::PredType::NATIVE_INT);
    }
    expect_error(path, "foo", "fewer instances", 2);

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = list_opener(handle, "foo");
        auto dhandle = ghandle.createGroup("data");
        auto ohandle1 = other_opener(dhandle, "0");
        write_scalar(ohandle1, "index", 0, H5::PredType::NATIVE_INT);
        auto ohandle2 = other_opener(dhandle, "1");
        write_scalar(ohandle2, "index", 0, H5::PredType::NATIVE_INT);
    }
    expect_error(path, "foo", "consecutive", 2);
}
