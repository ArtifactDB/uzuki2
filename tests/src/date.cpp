#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_hdf5.hpp"

#include "test_subclass.h"
#include "utils.h"

TEST(Hdf5Date, Legacy) {
    auto path = "TEST-date.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "date");
        write_strings(vhandle, "data", { "2077-12-12", "2055-01-01", "2022-05-06" });
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);
    auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_EQ(sptr->size(), 3);
    EXPECT_EQ(sptr->base.values.front(), "2077-12-12");
    EXPECT_EQ(sptr->base.values.back(), "2022-05-06");
    EXPECT_EQ(sptr->format, uzuki2::StringVector::DATE);
    EXPECT_FALSE(sptr->base.scalar);
}

TEST(Hdf5Date, Vector) {
    auto path = "TEST-date.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "string");
        add_version(vhandle, "1.2");
        write_strings(vhandle, "data", { "2077-12-12", "2055-01-01", "2022-05-06" });
        write_string(vhandle, "format", "date");
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);
    auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_EQ(sptr->size(), 3);
    EXPECT_EQ(sptr->base.values.front(), "2077-12-12");
    EXPECT_EQ(sptr->base.values.back(), "2022-05-06");
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

TEST(Hdf5Date, FormatError) {
    auto path = "TEST-date.h5";

    // Vector.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "date");
        write_strings(vhandle, "data", { "2077-12-12", /* invalid */ "2055-2-01", "2022-05-06" });
    }
    expect_hdf5_error(path, "foo", "dates should follow");

    // Scalar.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "date");
        write_string(vhandle, "data", "asda-as-as");
    }
    expect_hdf5_error(path, "foo", "dates should follow");
}

TEST(Hdf5Date, MissingPlaceholder) {
    auto path = "TEST-date.h5";

    // Check that the interaction between format checks and the missing placeholder is correct.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "date");
        auto dhandle = write_strings(vhandle, "data", { "2077-12-12", "NA", "NA" });

        H5::StrType stype(0, H5T_VARIABLE);
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", stype, H5S_SCALAR);
        ahandle.write(stype, std::string("NA"));
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);
    auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_EQ(sptr->size(), 3);
    EXPECT_EQ(sptr->base.values[1], "ich bin missing"); // i.e., the test's missing value placeholder.
    EXPECT_EQ(sptr->base.values[2], "ich bin missing");
    EXPECT_EQ(sptr->format, uzuki2::StringVector::DATE);
}

TEST(JsonDateTest, SimpleLoading) {
    {
        auto parsed = load_json("{ \"type\": \"date\", \"values\": [ \"2022-01-22\", \"1990-06-30\" ] }");
        EXPECT_EQ(parsed->type(), uzuki2::STRING);
        auto dptr = static_cast<const DefaultStringVector*>(parsed.get());
        EXPECT_EQ(dptr->size(), 2);
        EXPECT_FALSE(dptr->base.scalar);
        EXPECT_EQ(dptr->base.values[0], "2022-01-22");
        EXPECT_EQ(dptr->base.values[1], "1990-06-30");
        EXPECT_EQ(dptr->format, uzuki2::StringVector::DATE);
    }

    // Works with later versions.
    {
        auto parsed = load_json("{ \"type\": \"string\", \"values\": [ \"2022-01-22\", \"1990-06-30\" ], \"format\": \"date\", \"version\": \"1.1\" }");
        EXPECT_EQ(parsed->type(), uzuki2::STRING);
        auto dptr = static_cast<const DefaultStringVector*>(parsed.get());
        EXPECT_EQ(dptr->size(), 2);
        EXPECT_FALSE(dptr->base.scalar);
        EXPECT_EQ(dptr->base.values[0], "2022-01-22");
        EXPECT_EQ(dptr->base.values[1], "1990-06-30");
        EXPECT_EQ(dptr->format, uzuki2::StringVector::DATE);
    }

    // Works with scalars.
    {
        auto parsed = load_json("{ \"type\": \"string\", \"values\": \"2023-02-19\", \"format\":\"date\", \"version\":\"1.1\" }");
        EXPECT_EQ(parsed->type(), uzuki2::STRING);
        auto stuff = static_cast<const DefaultStringVector*>(parsed.get());
        EXPECT_TRUE(stuff->base.scalar);
        EXPECT_EQ(stuff->base.values[0], "2023-02-19");
        EXPECT_EQ(stuff->format, uzuki2::StringVector::DATE);
    }

    expect_json_error("{ \"type\": \"date\", \"values\": [ \"2022-01-22\", \"1990-06-30\" ], \"version\": \"1.1\" }", "unknown object type");

    /********************************************
     *** See integer.cpp for tests for names. ***
     ********************************************/
}

TEST(JsonDateTest, MissingValues) {
    auto parsed = load_json("{ \"type\": \"date\", \"values\": [ \"2022-01-22\", null ] }");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);
    auto dptr = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_EQ(dptr->size(), 2);
    EXPECT_EQ(dptr->base.values.back(), "ich bin missing");
    EXPECT_EQ(dptr->format, uzuki2::StringVector::DATE);
}

TEST(JsonDateTest, CheckError) {
    expect_json_error("{\"type\":\"date\", \"values\":[true,1,2] }", "expected a string");
    expect_json_error("{\"type\":\"date\", \"values\":[\"foo\", \"bar\"] }", "YYYY-MM-DD");
    expect_json_error("{\"type\":\"string\", \"format\":\"date\", \"values\":[\"foo\", \"bar\"], \"version\":\"1.1\"}", "YYYY-MM-DD");

    /***********************************************
     *** See integer.cpp for vector error tests. ***
     ***********************************************/
}
