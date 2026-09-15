#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <vector>
#include <string>

#include "uzuki2/parse_json.hpp"

#include "utils.h"
#include "../test_subclass.h"

TEST(JsonDateTime, Legacy) {
    std::vector<std::string> data{ "2022-01-22T00:00:00.1243Z", "1990-06-30T23:12:39.99+01:00" };
    auto parsed = load_json("{ \"type\": \"date-time\", \"values\": [ " + arrayify(data) + " ] }");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);

    auto dptr = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_EQ(dptr->base.values, data);
    EXPECT_EQ(dptr->format, uzuki2::StringVector::DATETIME);
}

TEST(JsonDateTime, Vector) {
    std::vector<std::string> data{ "2022-01-22T00:00:00.1243Z", "1990-06-30T23:12:39.99+01:00" };
    auto parsed = load_json("{ \"type\": \"string\", \"format\": \"date-time\", \"values\": [ " + arrayify(data) + " ], \"version\":\"1.1\"}");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);

    auto dptr = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_EQ(dptr->base.values, data);
    EXPECT_EQ(dptr->format, uzuki2::StringVector::DATETIME);
}

TEST(JsonDateTime, Scalar) {
    std::string val = "2023-02-19T12:34:56-09:00";
    auto parsed = load_json("{ \"type\": \"string\", \"format\":\"date-time\", \"values\": \"" + val + "\", \"version\":\"1.1\" }");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);

    auto stuff = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_TRUE(stuff->base.scalar);
    EXPECT_EQ(stuff->base.values[0], val);
    EXPECT_EQ(stuff->format, uzuki2::StringVector::DATETIME);
}

TEST(JsonDateTime, Missing) {
    std::string val = "2022-01-22T11:09:45.2-09:00";
    auto parsed = load_json("{ \"type\": \"date-time\", \"values\": [ \"" + val + "\", null ] }");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);

    auto dptr = static_cast<const DefaultStringVector*>(parsed.get());
    std::vector<std::string> expected{ val, "ich bin missing"};
    EXPECT_EQ(dptr->base.values, expected);
    EXPECT_EQ(dptr->format, uzuki2::StringVector::DATETIME);
}

TEST(JsonDateTime, Error) {
    expect_json_error("{\"type\":\"date-time\", \"values\":[\"foo\", \"bar\"] }", "Internet Date/Time");
    expect_json_error("{\"type\":\"string\", \"format\":\"date-time\", \"values\":[\"foo\", \"bar\"], \"version\":\"1.1\"}", "Internet Date/Time");
    expect_json_error("{ \"type\": \"date-time\", \"values\": [ \"2023-02-19T12:34:56-09:00\" ], \"version\": \"1.1\" }", "unknown object type");
}
