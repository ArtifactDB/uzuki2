#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_hdf5.hpp"

#include "utils.h"
#include "../test_subclass.h"

TEST(Hdf5DateTime, Legacy) {
    auto path = "TEST-datetime.h5";
    std::vector<std::string> data { 
        "2077-12-12T22:11:00Z", 
        "2055-01-01T05:34:12+19:11", 
        "2022-05-06T24:00:00-02:12", 
        "2022-05-06T24:00:00.000+02:12", 
        "2022-05-06T13:00:00.334-02:12"
    };

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "date-time");
        write_strings(vhandle, "data", data);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);
    auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_EQ(sptr->base.values, data);
    EXPECT_EQ(sptr->format, uzuki2::StringVector::DATETIME);
}

TEST(Hdf5DateTime, Vector) {
    auto path = "TEST-datetime.h5";
    std::vector<std::string> data { 
        "2077-12-12T22:11:00Z", 
        "2055-01-01T05:34:12+19:11", 
        "2022-05-06T24:00:00-02:12", 
        "2022-05-06T24:00:00.000+02:12", 
        "2022-05-06T13:00:00.334-02:12"
    };

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "string");
        add_version(vhandle, "1.1");
        write_strings(vhandle, "data", data);
        write_string(vhandle, "format", "date-time");
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);
    auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_EQ(sptr->base.values, data);
    EXPECT_EQ(sptr->format, uzuki2::StringVector::DATETIME);
}

TEST(Hdf5DateTime, Scalar) {
    auto path = "TEST-datetime.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "string");
        add_version(vhandle, "1.1");
        write_string(vhandle, "data", "2077-12-12T22:11:00Z");
        write_string(vhandle, "format", "date-time");
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);
    auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_EQ(sptr->size(), 1);
    EXPECT_EQ(sptr->base.values.front(), "2077-12-12T22:11:00Z");
    EXPECT_EQ(sptr->format, uzuki2::StringVector::DATETIME);
}

TEST(Hdf5DateTime, FormatErrorScalar) {
    auto path = "TEST-datetime.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "date-time");
        write_string(vhandle, "data", "2022-02-20");
    }
    expect_hdf5_error(path, "foo", "date-times should follow");
}

TEST(Hdf5DateTime, FormatErrorVector) {
    auto path = "TEST-datetime.h5";
    std::vector<std::string> data { 
        "2077-12-12T22:11:00Z", 
        "2055-01-01T05:34:12+19:11", 
        "2022-05-06T24:00:00-02:12", 
        "2022-05-06T24:00:00.000+02:12", 
        "2022-05-06T13:00:00.334-02:12"
    };

    // Invalid date occurs in the first chunk.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "date-time");
        auto modified = data;
        modified[0] = "2055-02-01";
        write_strings(vhandle, "data", modified, /* variable = */ false, /* chunk_size = */ 2);
    }
    expect_hdf5_error(path, "foo", "date-times should follow");

    // Now in the last chunk.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "date-time");
        auto modified = data;
        modified.back() = "NA";
        write_strings(vhandle, "data", modified, /* variable = */ false, /* chunk_size = */ 2);
    }
    expect_hdf5_error(path, "foo", "date-times should follow");
}

TEST(Hdf5DateTime, MissingPlaceholder) {
    auto path = "TEST-datetime.h5";

    // Check that the interaction between format checks and the missing placeholder is correct.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "date-time");
        auto dhandle = write_strings(vhandle, "data", { "2077-12-12T22:11:00Z", "NA", "NA" });

        H5::StrType stype(0, H5T_VARIABLE);
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", stype, H5S_SCALAR);
        std::string target = "NA";
        ahandle.write(stype, target);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);
    auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_EQ(sptr->size(), 3);
    EXPECT_EQ(sptr->base.values[1], "ich bin missing"); // i.e., the test's missing value placeholder.
    EXPECT_EQ(sptr->base.values[2], "ich bin missing");
    EXPECT_EQ(sptr->format, uzuki2::StringVector::DATETIME);
}
