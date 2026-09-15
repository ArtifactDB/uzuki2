#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <vector>
#include <string>

#include "uzuki2/parse_json.hpp"

#include "utils.h"
#include "../test_subclass.h"

TEST(JsonDate, Legacy) {
    std::vector<std::string> data{ "2022-01-22", "1990-06-30" };
    auto parsed = load_json("{ \"type\": \"date\", \"values\": [ " + arrayify(data) + " ] }");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);

    auto dptr = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_FALSE(dptr->base.scalar);
    EXPECT_EQ(dptr->base.values, data);
    EXPECT_EQ(dptr->format, uzuki2::StringVector::DATE);
}

TEST(JsonDate, Vector) {
    std::vector<std::string> data{ "2022-01-22", "1990-06-30" };
    auto parsed = load_json("{ \"type\": \"string\", \"values\": [ " + arrayify(data) + " ], \"format\": \"date\", \"version\": \"1.1\" }");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);

    auto dptr = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_FALSE(dptr->base.scalar);
    EXPECT_EQ(dptr->base.values, data);
    EXPECT_EQ(dptr->format, uzuki2::StringVector::DATE);
}

TEST(JsonDate, Scalar) {
    std::string val = "2023-02-19";
    auto parsed = load_json("{ \"type\": \"string\", \"values\": \"" + val + "\", \"format\": \"date\", \"version\":\"1.1\" }");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);

    auto stuff = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_TRUE(stuff->base.scalar);
    EXPECT_EQ(stuff->base.values[0], val);
    EXPECT_EQ(stuff->format, uzuki2::StringVector::DATE);
}

TEST(JsonDate, Missing) {
    auto parsed = load_json("{ \"type\": \"date\", \"values\": [ \"2022-01-22\", null ] }");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);

    auto dptr = static_cast<const DefaultStringVector*>(parsed.get());
    std::vector<std::string> expected{ "2022-01-22", "ich bin missing" };
    EXPECT_EQ(dptr->base.values, expected);
    EXPECT_EQ(dptr->format, uzuki2::StringVector::DATE);
}

TEST(JsonDate, Error) {
    expect_json_error("{\"type\":\"date\", \"values\":[\"foo\", \"bar\"] }", "YYYY-MM-DD");
    expect_json_error("{\"type\":\"string\", \"format\":\"date\", \"values\":[\"foo\", \"bar\"], \"version\":\"1.1\"}", "YYYY-MM-DD");
    expect_json_error("{ \"type\": \"date\", \"values\": [ \"2022-01-22\", \"1990-06-30\" ], \"version\": \"1.1\" }", "unknown object type");
}
