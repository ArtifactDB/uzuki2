#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_hdf5.hpp"

#include "test_subclass.h"
#include "utils.h"

TEST(Hdf5StringTest, SimpleLoading) {
    auto path = "TEST-string.h5";

    // Simple stuff works correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "string");
        create_dataset(vhandle, "data", { "foo", "whee", "stuff" });
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::STRING);
        auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
        EXPECT_EQ(sptr->size(), 3);
        EXPECT_EQ(sptr->base.values.front(), "foo");
        EXPECT_EQ(sptr->base.values.back(), "stuff");
        EXPECT_FALSE(sptr->base.scalar);
        EXPECT_EQ(sptr->format, uzuki2::StringVector::NONE);
    }

    // Variable stuff works correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "string");
        create_dataset(vhandle, "data", { "foo-qwerty", "whee", "stuff-asdasd" }, true);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::STRING);
        auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
        EXPECT_EQ(sptr->size(), 3);
        EXPECT_EQ(sptr->base.values.front(), "foo-qwerty");
        EXPECT_EQ(sptr->base.values.back(), "stuff-asdasd");
        EXPECT_EQ(sptr->format, uzuki2::StringVector::NONE);
    }

    // Scalars work correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "string");
        write_string(vhandle, "data", "antony");
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::STRING);
        auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
        EXPECT_EQ(sptr->size(), 1);
        EXPECT_EQ(sptr->base.values.front(), "antony");
        EXPECT_TRUE(sptr->base.scalar);
        EXPECT_EQ(sptr->format, uzuki2::StringVector::NONE);
    }

    // Works with recent versions.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "string");
        add_version(vhandle, "1.1");
        create_dataset(vhandle, "data", { "foo", "whee", "stuff" });
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::STRING);
        auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
        EXPECT_EQ(sptr->size(), 3);
        EXPECT_EQ(sptr->format, uzuki2::StringVector::NONE);
    }

    /********************************************
     *** See integer.cpp for tests for names. ***
     ********************************************/
}

TEST(Hdf5StringTest, MissingValues) {
    auto path = "TEST-string.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "string");
        auto dhandle = create_dataset(vhandle, "data", { "michael", "gabriel", "raphael", "lucifer" });

        H5::StrType stype(0, H5T_VARIABLE);
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", stype, H5S_SCALAR);
        std::string target = "lucifer";
        ahandle.write(stype, target);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::STRING);
        auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
        EXPECT_EQ(sptr->size(), 4);
        EXPECT_EQ(sptr->base.values[3], "ich bin missing"); // i.e., the test's missing value placeholder.
    }
}

TEST(Hdf5StringTest, CheckError) {
    auto path = "TEST-string.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "string");
        create_dataset<int>(ghandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT);
    }
    expect_hdf5_error(path, "foo", "expected a string");

    /***********************************************
     *** See integer.cpp for vector error tests. ***
     ***********************************************/
}

TEST(JsonStringTest, SimpleLoading) {
    {
        auto parsed = load_json("{\"type\":\"string\", \"values\":[\"alpha\", \"bravo\", \"charlie\"] }");
        EXPECT_EQ(parsed->type(), uzuki2::STRING);
        auto bptr = static_cast<const DefaultStringVector*>(parsed.get());
        EXPECT_EQ(bptr->size(), 3);
        EXPECT_FALSE(bptr->base.scalar);
        EXPECT_EQ(bptr->base.values.front(), "alpha");
        EXPECT_EQ(bptr->base.values.back(), "charlie");
        EXPECT_EQ(bptr->format, uzuki2::StringVector::NONE);
    }

    // Works with scalars.
    {
        auto parsed = load_json("{ \"type\": \"string\", \"values\": \"foo\" }");
        EXPECT_EQ(parsed->type(), uzuki2::STRING);
        auto stuff = static_cast<const DefaultStringVector*>(parsed.get());
        EXPECT_TRUE(stuff->base.scalar);
        EXPECT_EQ(stuff->base.values[0], "foo");
        EXPECT_EQ(stuff->format, uzuki2::StringVector::NONE);
    }

    // Works with recent versions.
    {
        auto parsed = load_json("{\"type\":\"string\", \"values\":[\"alpha\", \"bravo\", \"charlie\"], \"version\": \"1.1\" }");
        EXPECT_EQ(parsed->type(), uzuki2::STRING);
        auto bptr = static_cast<const DefaultStringVector*>(parsed.get());
        EXPECT_EQ(bptr->size(), 3);
        EXPECT_EQ(bptr->format, uzuki2::StringVector::NONE);
    }

    /********************************************
     *** See integer.cpp for tests for names. ***
     ********************************************/
}

TEST(JsonStringTest, MissingValues) {
    auto parsed = load_json("{\"type\":\"string\", \"values\":[\"alpha\", null, null, \"delta\", \"echo\"] }");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);

    auto bptr = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_EQ(bptr->size(), 5);
    EXPECT_EQ(bptr->base.values[1], "ich bin missing");
    EXPECT_EQ(bptr->base.values[2], "ich bin missing");
}

TEST(JsonStringTest, CheckError) {
    expect_json_error("{ \"type\": \"string\", \"values\": [true]}", "expected a string");
    expect_json_error("{\"type\":\"string\", \"format\":2, \"values\":[\"foo\", \"bar\"], \"version\":\"1.1\"}", "expected a string");
    expect_json_error("{\"type\":\"string\", \"format\":\"whee\", \"values\":[\"foo\", \"bar\"], \"version\":\"1.1\"}", "unsupported format");

    /***********************************************
     *** See integer.cpp for vector error tests. ***
     ***********************************************/
}
