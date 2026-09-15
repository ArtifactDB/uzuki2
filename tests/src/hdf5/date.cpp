#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_hdf5.hpp"

#include "utils.h"
#include "../test_subclass.h"

TEST(Hdf5Date, Legacy) {
    auto path = "TEST-date.h5";
    std::vector<std::string> data{ "2077-12-12", "2055-01-01", "2022-05-06" };

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "date");
        write_strings(vhandle, "data", data);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);
    auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_EQ(sptr->size(), 3);
    EXPECT_EQ(sptr->base.values, data);
    EXPECT_EQ(sptr->format, uzuki2::StringVector::DATE);
    EXPECT_FALSE(sptr->base.scalar);
}

TEST(Hdf5Date, Vector) {
    auto path = "TEST-date.h5";
    std::vector<std::string> data{ "2077-12-12", "2055-01-01", "2022-05-06" };

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "string");
        add_version(vhandle, "1.2");
        write_strings(vhandle, "data", data);
        write_string(vhandle, "format", "date");
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);
    auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_EQ(sptr->size(), 3);
    EXPECT_EQ(sptr->base.values, data);
    EXPECT_EQ(sptr->format, uzuki2::StringVector::DATE);
    EXPECT_FALSE(sptr->base.scalar);
}

TEST(Hdf5Date, Scalar) {
    auto path = "TEST-date.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "string");
        add_version(vhandle, "1.2");
        write_string(vhandle, "data", "2077-12-12");
        write_string(vhandle, "format", "date");
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);
    auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_EQ(sptr->size(), 1);
    EXPECT_EQ(sptr->base.values.front(), "2077-12-12");
    EXPECT_EQ(sptr->format, uzuki2::StringVector::DATE);
    EXPECT_TRUE(sptr->base.scalar);
}

TEST(Hdf5Date, FormatErrorScalar) {
    auto path = "TEST-date.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "date");
        write_string(vhandle, "data", "asda-as-as");
    }
    expect_hdf5_error(path, "foo", "dates should follow");
}

TEST(Hdf5Date, FormatErrorVector) {
    auto path = "TEST-date.h5";
    std::vector<std::string> data{ "2077-12-12", "2055-01-01", "2022-05-06", "2032-07-15" };

    // Invalid date occurs in the first chunk.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "date");
        auto modified = data;
        modified[0] = "2055-2-01";
        write_strings(vhandle, "data", modified, /* variable = */ false, /* chunk_size = */ 2);
    }
    expect_hdf5_error(path, "foo", "dates should follow");

    // Now in the last chunk.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "date");
        auto modified = data;
        modified.back() = "NA";
        write_strings(vhandle, "data", modified, /* variable = */ false, /* chunk_size = */ 2);
    }
    expect_hdf5_error(path, "foo", "dates should follow");
}

TEST(Hdf5Date, MissingPlaceholder) {
    auto path = "TEST-date.h5";
    std::vector<std::string> data{ "2077-12-12", "NA", "NA" };

    // Check that the interaction between format checks and the missing placeholder is correct.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "date");
        auto dhandle = write_strings(vhandle, "data", data);
        H5::StrType stype(0, H5T_VARIABLE);
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", stype, H5S_SCALAR);
        ahandle.write(stype, std::string("NA"));
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);
    auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
    auto expected = data;
    expected[1] = "ich bin missing"; // i.e., the test's missing value placeholder.
    expected[2] = "ich bin missing";
    EXPECT_EQ(sptr->base.values, expected);
    EXPECT_EQ(sptr->format, uzuki2::StringVector::DATE);
}
