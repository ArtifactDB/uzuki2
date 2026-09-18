#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_hdf5.hpp"

#include "utils.h"
#include "../test_subclass.h"

TEST(Hdf5List, SimpleLoading) {
    auto path = "TEST-list.h5";

    // Simple stuff works correctly.
    std::vector<std::int32_t> expected{ -1, -2, -3, 0, 1, 2, 3 };
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = list_opener(handle, "foo");
        auto dhandle = ghandle.createGroup("data");
        nothing_opener(dhandle, "0");
        auto vhandle = vector_opener(dhandle, "1", "integer");
        write_numbers(vhandle, "data", expected, H5::PredType::NATIVE_INT8);
    }

    auto parsed = load_hdf5_strict(path, "foo");
    EXPECT_EQ(parsed->type(), uzuki2::LIST);

    auto stuff = static_cast<const DefaultList*>(parsed.get());
    EXPECT_EQ(stuff->size(), 2);
    EXPECT_EQ(stuff->values[0]->type(), uzuki2::NOTHING);
    EXPECT_EQ(stuff->values[1]->type(), uzuki2::INTEGER);

    auto iptr = static_cast<const DefaultIntegerVector*>(stuff->values[1].get());
    EXPECT_EQ(iptr->base.values, expected);

    // Test coverage of the relevant Dummy class.
    validate_hdf5(path, "foo");
}

TEST(Hdf5List, NestedLoading) {
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

    auto parsed = load_hdf5_strict(path, "foo");
    EXPECT_EQ(parsed->type(), uzuki2::LIST);

    auto stuff = static_cast<const DefaultList*>(parsed.get());
    EXPECT_EQ(stuff->size(), 2);

    EXPECT_EQ(stuff->values[0]->type(), uzuki2::NOTHING);
    EXPECT_EQ(stuff->values[1]->type(), uzuki2::LIST);

    auto lptr = static_cast<const DefaultList*>(stuff->values[1].get());
    EXPECT_EQ(lptr->size(), 1);
}

TEST(Hdf5List, CheckError) {
    auto path = "TEST-list.h5";
    H5Eset_auto2(H5E_DEFAULT, NULL, NULL);

    // All 'N' children of the list's 'data/' group should be named 0, 1, 2, ... N -1.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = list_opener(handle, "foo");
        auto dhandle = ghandle.createGroup("data");
        nothing_opener(dhandle, "1");
    }
    bool failed = true;
    try {
        uzuki2::hdf5::validate(path, "foo", 0, {});
    } catch (H5::Exception&) {
        failed = true;
    }
    EXPECT_TRUE(failed);

    // Catches and rethrows nested errors correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = list_opener(handle, "foo");
        auto dhandle = ghandle.createGroup("data");
        std::map<std::string, std::string> attrs;
        attrs["uzuki_object"] = "bar";
        super_group_opener(dhandle, "0", attrs);
    }
    expect_hdf5_error(path, "foo", "unknown");
}
