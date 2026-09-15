#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_json.hpp"

#include "utils.h"
#include "../test_subclass.h"

TEST(JsonString, Vector) {
    std::vector<std::string> data{  "alpha", "bravo", "charlie" };

    {
        auto parsed = load_json("{\"type\":\"string\", \"values\":[ " + arrayify(data) + " ] }");
        EXPECT_EQ(parsed->type(), uzuki2::STRING);
        auto bptr = static_cast<const DefaultStringVector*>(parsed.get());

        EXPECT_FALSE(bptr->base.scalar);
        EXPECT_EQ(bptr->base.values, data);
        EXPECT_EQ(bptr->format, uzuki2::StringVector::NONE);
    }

    // Also testing more recent versions to get some coverage of the no-'format' case.
    {
        auto parsed = load_json("{\"type\":\"string\", \"values\":[ " + arrayify(data) + "], \"version\": \"1.1\" }");
        EXPECT_EQ(parsed->type(), uzuki2::STRING);
        auto bptr = static_cast<const DefaultStringVector*>(parsed.get());
        EXPECT_EQ(bptr->size(), 3);
        EXPECT_EQ(bptr->format, uzuki2::StringVector::NONE);
    }
}

TEST(JsonString, Scalar) {
    auto parsed = load_json("{ \"type\": \"string\", \"values\": \"foo\" }");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);

    auto stuff = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_TRUE(stuff->base.scalar);
    EXPECT_EQ(stuff->base.values[0], "foo");
    EXPECT_EQ(stuff->format, uzuki2::StringVector::NONE);
}

TEST(JsonStringTest, Missing) {
    auto parsed = load_json("{\"type\":\"string\", \"values\":[\"alpha\", null, null, \"delta\", \"echo\"] }");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);

    auto bptr = static_cast<const DefaultStringVector*>(parsed.get());
    std::vector<std::string> expected{  "alpha", "ich bin missing", "ich bin missing", "delta", "echo" };
    EXPECT_EQ(bptr->base.values, expected);
}

TEST(JsonStringTest, Error) {
    expect_json_error("{ \"type\": \"string\", \"values\": [true]}", "expected a string");
}
