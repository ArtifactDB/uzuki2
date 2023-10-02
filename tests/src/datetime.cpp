#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_hdf5.hpp"

#include "test_subclass.h"
#include "utils.h"

TEST(Hdf5DateTimeTest, SimpleLoading) {
    auto path = "TEST-datetime.h5";

    // Simple stuff works correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "date-time");
        create_dataset(vhandle, "data", { 
            "2077-12-12T22:11:00Z", 
            "2055-01-01T05:34:12+19:11", 
            "2022-05-06T24:00:00-02:12", 
            "2022-05-06T24:00:00.000+02:12", 
            "2022-05-06T13:00:00.334-02:12"
        });
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::STRING);
        auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
        EXPECT_EQ(sptr->size(), 5);
        EXPECT_EQ(sptr->base.values.front(), "2077-12-12T22:11:00Z");
        EXPECT_EQ(sptr->base.values.back(), "2022-05-06T13:00:00.334-02:12");
        EXPECT_EQ(sptr->format, uzuki2::StringVector::DATETIME);
    }

    // Works for scalars.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "date-time");
        write_string(vhandle, "data", "2077-12-12T22:11:00Z");
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::STRING);
        auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
        EXPECT_EQ(sptr->size(), 1);
        EXPECT_EQ(sptr->base.values.front(), "2077-12-12T22:11:00Z");
        EXPECT_EQ(sptr->format, uzuki2::StringVector::DATETIME);
    }

    // Works for the most recent version.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "string");
        add_version(vhandle, "1.1");

        create_dataset(vhandle, "data", { 
            "2077-12-12T22:11:00Z", 
            "2055-01-01T05:34:12+19:11", 
            "2022-05-06T24:00:00-02:12", 
            "2022-05-06T24:00:00.000+02:12", 
            "2022-05-06T13:00:00.334-02:12"
        });
        write_string(vhandle, "format", "date-time");
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::STRING);
        auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
        EXPECT_EQ(sptr->size(), 5);
        EXPECT_EQ(sptr->base.values.front(), "2077-12-12T22:11:00Z");
        EXPECT_EQ(sptr->base.values.back(), "2022-05-06T13:00:00.334-02:12");
        EXPECT_EQ(sptr->format, uzuki2::StringVector::DATETIME);
    }

    /********************************************
     *** See integer.cpp for tests for names. ***
     ********************************************/
}

TEST(Hdf5DateTimeTest, MissingValues) {
    auto path = "TEST-datetime.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "date-time");
        auto dhandle = create_dataset(vhandle, "data", { "2077-12-12T22:11:00Z", "NA", "NA" });

        H5::StrType stype(0, H5T_VARIABLE);
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", stype, H5S_SCALAR);
        std::string target = "NA";
        ahandle.write(stype, target);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::STRING);
        auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
        EXPECT_EQ(sptr->size(), 3);
        EXPECT_EQ(sptr->base.values[2], "ich bin missing"); // i.e., the test's missing value placeholder.
        EXPECT_EQ(sptr->format, uzuki2::StringVector::DATETIME);
    }
}

TEST(Hdf5DateTimeTest, CheckError) {
    auto path = "TEST-datetime.h5";

    std::vector<std::string> invalid_dates {
        "2055-99-01T05:34:12",
        "2055-99-01T05:34:12+19:11", 
        "2055-99-01T05:34:12",
        "2055-12-01T11:5511Z",
        "2055-12-01T1155:11-09:55",
        "2055-12-01T29:12:11-09:55",
        "2055-12-01T30:12:11-09:55",
        "2055-12-01T23:60:11-09:55",
        "2055-12-01T23:60:11-09:55",
        "2055-12-01T24:01:01Z",
        "2055-12-01T24:00:00.Z",
        "2055-12-01T24:00:00Z.",
        "2055-12-01T24:00:00A",
        "2055-12-01T24:00:00.A",
        "2055-12-01T12:00:00-99",
        "2055-12-01T12:00:00-99:55",
        "2055-12-01T12:00:00-00:99"
    };

    for (const auto& x : invalid_dates) {
        {
            H5::H5File handle(path, H5F_ACC_TRUNC);
            auto vhandle = vector_opener(handle, "foo", "date-time");
            create_dataset(vhandle, "data", { x });
        }
        expect_hdf5_error(path, "foo", "date-times should follow");
    }

    for (const auto& x : invalid_dates) {
        {
            H5::H5File handle(path, H5F_ACC_TRUNC);
            auto vhandle = vector_opener(handle, "foo", "string");
            write_string(vhandle, "format", "date-time");
            add_version(vhandle, "1.1");
            create_dataset(vhandle, "data", { x });
        }
        expect_hdf5_error(path, "foo", "date-times should follow");
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "date-time");
        add_version(vhandle, "1.1");
        create_dataset(vhandle, "data", { "2077-12-12T00:00:00Z" });
    }
    expect_hdf5_error(path, "foo", "unknown vector type");

    /***********************************************
     *** See integer.cpp for vector error tests. ***
     ***********************************************/
}

TEST(JsonDateTimeTest, SimpleLoading) {
    {
        auto parsed = load_json("{ \"type\": \"date-time\", \"values\": [ \"2022-01-22T00:00:00.1243Z\", \"1990-06-30T23:12:39.99+01:00\" ] }");
        EXPECT_EQ(parsed->type(), uzuki2::STRING);
        auto dptr = static_cast<const DefaultStringVector*>(parsed.get());
        EXPECT_EQ(dptr->size(), 2);
        EXPECT_EQ(dptr->base.values[0], "2022-01-22T00:00:00.1243Z");
        EXPECT_EQ(dptr->base.values[1], "1990-06-30T23:12:39.99+01:00");
        EXPECT_EQ(dptr->format, uzuki2::StringVector::DATETIME);
    }

    // Works with a more recent version.
    {
        auto parsed = load_json("{ \"type\":\"string\", \"format\":\"date-time\", \"values\": [ \"2022-01-22T00:00:00.1243Z\", \"1990-06-30T23:12:39.99+01:00\" ], \"version\":\"1.1\"}");
        EXPECT_EQ(parsed->type(), uzuki2::STRING);
        auto dptr = static_cast<const DefaultStringVector*>(parsed.get());
        EXPECT_EQ(dptr->size(), 2);
        EXPECT_EQ(dptr->base.values[0], "2022-01-22T00:00:00.1243Z");
        EXPECT_EQ(dptr->base.values[1], "1990-06-30T23:12:39.99+01:00");
        EXPECT_EQ(dptr->format, uzuki2::StringVector::DATETIME);
    }

    // Works with scalars.
    {
        auto parsed = load_json("{ \"type\": \"string\", \"format\":\"date-time\", \"values\": \"2023-02-19T12:34:56-09:00\", \"version\":\"1.1\" }");
        EXPECT_EQ(parsed->type(), uzuki2::STRING);
        auto stuff = static_cast<const DefaultStringVector*>(parsed.get());
        EXPECT_TRUE(stuff->scalar);
        EXPECT_EQ(stuff->base.values[0], "2023-02-19T12:34:56-09:00");
        EXPECT_EQ(stuff->format, uzuki2::StringVector::DATETIME);
    }

    expect_json_error("{ \"type\": \"date-time\", \"values\": [ \"2023-02-19T12:34:56-09:00\" ], \"version\": \"1.1\" }", "unknown object type");

    /********************************************
     *** See integer.cpp for tests for names. ***
     ********************************************/
}

TEST(JsonDateTimeTest, MissingValues) {
    auto parsed = load_json("{ \"type\": \"date-time\", \"values\": [ \"2022-01-22T11:09:45.2-09:00\", null ] }");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);
    auto dptr = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_EQ(dptr->size(), 2);
    EXPECT_EQ(dptr->base.values.back(), "ich bin missing");
    EXPECT_EQ(dptr->format, uzuki2::StringVector::DATETIME);
}

TEST(JsonDateTimeTest, CheckError) {
    expect_json_error("{\"type\":\"date-time\", \"values\":[true,1,2] }", "expected a string");
    expect_json_error("{\"type\":\"date-time\", \"values\":[\"foo\", \"bar\"] }", "Internet Date/Time");
    expect_json_error("{\"type\":\"string\", \"format\":\"date-time\", \"values\":[\"foo\", \"bar\"], \"version\":\"1.1\"}", "Internet Date/Time");

    /***********************************************
     *** See integer.cpp for vector error tests. ***
     ***********************************************/
}
