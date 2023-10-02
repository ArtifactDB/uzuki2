#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_hdf5.hpp"

#include "test_subclass.h"
#include "utils.h"

TEST(Hdf5DateTest, SimpleLoading) {
    auto path = "TEST-date.h5";

    // Simple stuff works correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "date");
        create_dataset(vhandle, "data", { "2077-12-12", "2055-01-01", "2022-05-06" });
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::DATE);
        auto sptr = static_cast<const DefaultDateVector*>(parsed.get());
        EXPECT_EQ(sptr->size(), 3);
        EXPECT_EQ(sptr->base.values.front(), "2077-12-12");
        EXPECT_EQ(sptr->base.values.back(), "2022-05-06");
        EXPECT_FALSE(sptr->scalar);
    }

    // Scalars work correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "date");
        write_string(vhandle, "data", "2022-05-09");
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::DATE);
        auto sptr = static_cast<const DefaultDateVector*>(parsed.get());
        EXPECT_EQ(sptr->size(), 1);
        EXPECT_EQ(sptr->base.values.front(), "2022-05-09");
        EXPECT_TRUE(sptr->scalar);
    }

    /********************************************
     *** See integer.cpp for tests for names. ***
     ********************************************/
}

TEST(Hdf5DateTest, MissingValues) {
    auto path = "TEST-date.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "date");
        auto dhandle = create_dataset(vhandle, "data", { "2077-12-12", "NA", "NA" });

        H5::StrType stype(0, H5T_VARIABLE);
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", stype, H5S_SCALAR);
        std::string target = "NA";
        ahandle.write(stype, target);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::DATE);
        auto sptr = static_cast<const DefaultDateVector*>(parsed.get());
        EXPECT_EQ(sptr->size(), 3);
        EXPECT_EQ(sptr->base.values[2], "ich bin missing"); // i.e., the test's missing value placeholder.
    }
}

TEST(Hdf5DateTest, CheckError) {
    auto path = "TEST-date.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "date");
        create_dataset(vhandle, "data", { "2077-12-12", "2055-23-01", "2022-05-06" });
    }
    expect_hdf5_error(path, "foo", "dates should follow");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "date");
        create_dataset(vhandle, "data", { "2077-12-12", "2055-2-01", "2022-05-06" });
    }
    expect_hdf5_error(path, "foo", "dates should follow");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "date");
        create_dataset(vhandle, "data", { "2077-12-12", "2055-12-1", "2022-05-06" });
    }
    expect_hdf5_error(path, "foo", "dates should follow");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "date");
        create_dataset(vhandle, "data", { "2077-12-12", "22-12-12", "2022-05-06" });
    }
    expect_hdf5_error(path, "foo", "dates should follow");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "date");
        create_dataset(vhandle, "data", { "2077-12-12", "2022-12-35", "2022-05-06" });
    }
    expect_hdf5_error(path, "foo", "dates should follow");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "date");
        create_dataset(vhandle, "data", { "2077-12-12", "2022-12-55", "2022-05-06" });
    }
    expect_hdf5_error(path, "foo", "dates should follow");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "date");
        create_dataset(vhandle, "data", { "2077-12-12", "asda-sd-as", "2022-05-06" });
    }
    expect_hdf5_error(path, "foo", "dates should follow");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "date");
        create_dataset(vhandle, "data", { "harry", "ron", "hermoine" });
    }
    expect_hdf5_error(path, "foo", "dates should follow");


    /***********************************************
     *** See integer.cpp for vector error tests. ***
     ***********************************************/
}

TEST(JsonDateTest, SimpleLoading_v1_1) {
    auto parsed = load_json("{ \"type\": \"string\", \"values\": [ \"2022-01-22\", \"1990-06-30\" ], \"format\": \"date\", \"version\": \"1.1\" }");
    EXPECT_EQ(parsed->type(), uzuki2::DATE);
    auto dptr = static_cast<const DefaultDateVector*>(parsed.get());
    EXPECT_EQ(dptr->size(), 2);
    EXPECT_FALSE(dptr->scalar);
    EXPECT_EQ(dptr->base.values[0], "2022-01-22");
    EXPECT_EQ(dptr->base.values[1], "1990-06-30");

    // Works with scalars.
    {
        auto parsed = load_json("{ \"type\": \"string\", \"values\": \"2023-02-19\", \"format\":\"date\", \"version\":\"1.1\" }");
        EXPECT_EQ(parsed->type(), uzuki2::DATE);
        auto stuff = static_cast<const DefaultDateVector*>(parsed.get());
        EXPECT_TRUE(stuff->scalar);
        EXPECT_EQ(stuff->base.values[0], "2023-02-19");
    }

    expect_json_error("{ \"type\": \"date\", \"values\": [ \"2022-01-22\", \"1990-06-30\" ], \"version\": \"1.1\" }", "unknown object type");

    /********************************************
     *** See integer.cpp for tests for names. ***
     ********************************************/
}

TEST(JsonDateTest, SimpleLoading_v1_0) {
    auto parsed = load_json("{ \"type\": \"date\", \"values\": [ \"2022-01-22\", \"1990-06-30\" ] }");
    EXPECT_EQ(parsed->type(), uzuki2::DATE);
    auto dptr = static_cast<const DefaultDateVector*>(parsed.get());
    EXPECT_EQ(dptr->size(), 2);
    EXPECT_FALSE(dptr->scalar);
    EXPECT_EQ(dptr->base.values[0], "2022-01-22");
    EXPECT_EQ(dptr->base.values[1], "1990-06-30");
}

TEST(JsonDateTest, MissingValues) {
    auto parsed = load_json("{ \"type\": \"date\", \"values\": [ \"2022-01-22\", null ] }");
    EXPECT_EQ(parsed->type(), uzuki2::DATE);
    auto dptr = static_cast<const DefaultDateVector*>(parsed.get());
    EXPECT_EQ(dptr->size(), 2);
    EXPECT_EQ(dptr->base.values.back(), "ich bin missing");
}

TEST(JsonDateTest, CheckError) {
    expect_json_error("{\"type\":\"date\", \"values\":[true,1,2] }", "expected a string");
    expect_json_error("{\"type\":\"date\", \"values\":[\"foo\", \"bar\"] }", "YYYY-MM-DD");
    expect_json_error("{\"type\":\"string\", \"format\":\"date\", \"values\":[\"foo\", \"bar\"], \"version\":\"1.1\"}", "YYYY-MM-DD");

    /***********************************************
     *** See integer.cpp for vector error tests. ***
     ***********************************************/
}
