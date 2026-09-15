#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_hdf5.hpp"

#include "utils.h"
#include "../test_subclass.h"

TEST(JsonNames, Vector) {
    std::vector<std::int32_t> data{ 1, -1, 2, -2, 3, -3 };
    std::vector<std::string> names { "a", "bb", "ccc", "dddd", "eeeee", "ffffff" };
    auto parsed = load_json("{ \"type\": \"integer\", \"values\": [ " + arrayify(data) + " ], \"names\": [ " + arrayify(names) + " ] }");
    EXPECT_EQ(parsed->type(), uzuki2::INTEGER);

    auto stuff = static_cast<const DefaultIntegerVector*>(parsed.get());
    EXPECT_EQ(stuff->base.values, data);
    EXPECT_TRUE(stuff->base.has_names);
    EXPECT_EQ(stuff->base.names, names);
}

TEST(JsonNames, Scalar) {
    auto parsed = load_json("{ \"type\": \"string\", \"values\": \"foo\", \"names\": [ \"bar\" ] }");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);

    auto stuff = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_TRUE(stuff->base.scalar);
    EXPECT_EQ(stuff->base.values.size(), 1);
    EXPECT_EQ(stuff->base.values[0], "foo");
    EXPECT_TRUE(stuff->base.has_names);
    EXPECT_EQ(stuff->base.names.size(), 1);
    EXPECT_EQ(stuff->base.names[0], "bar");
}

TEST(JsonNames, Error) {
    expect_json_error("{ \"type\": \"integer\", \"values\": [-99], \"names\": true}", "expected an array");
    expect_json_error("{ \"type\": \"integer\", \"values\": [-99], \"names\": [5]}", "expected a string");
    expect_json_error("{ \"type\": \"integer\", \"values\": [-99], \"names\": [\"a\", \"b\"]}", "should be the same");
}

TEST(JsonNames, List) {
    auto parsed = load_json_strict("{ \"type\":\"list\", \"values\": [ { \"type\": \"nothing\" }, { \"type\": \"integer\", \"values\": [ 1, 2, 3 ] } ], \"names\": [ \"X\", \"Y\" ] }");
    EXPECT_EQ(parsed->type(), uzuki2::LIST);

    auto stuff = static_cast<const DefaultList*>(parsed.get());
    EXPECT_TRUE(stuff->has_names);
    EXPECT_EQ(stuff->names.size(), 2);
    EXPECT_EQ(stuff->names[0], "X");
    EXPECT_EQ(stuff->names[1], "Y");
}


