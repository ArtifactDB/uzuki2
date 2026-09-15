#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_json.hpp"

#include "utils.h"
#include "../test_subclass.h"

TEST(JsonList, Simple) {
    std::vector<std::int32_t> data{ 1, 2, 3 };
    auto parsed = load_json_strict("{ \"type\":\"list\", \"values\": [ { \"type\": \"nothing\" }, { \"type\": \"integer\", \"values\": [ " + arrayify(data) + " ] } ] }");
    EXPECT_EQ(parsed->type(), uzuki2::LIST);

    auto stuff = static_cast<const DefaultList*>(parsed.get());
    EXPECT_EQ(stuff->size(), 2);

    EXPECT_EQ(stuff->values[0]->type(), uzuki2::NOTHING);
    EXPECT_EQ(stuff->values[1]->type(), uzuki2::INTEGER);

    auto iptr = static_cast<const DefaultIntegerVector*>(stuff->values[1].get());
    EXPECT_EQ(iptr->base.values, data);
}

TEST(JsonList, Empty) {
    auto parsed = load_json_strict("{ \"type\":\"list\", \"values\": [] }");
    EXPECT_EQ(parsed->type(), uzuki2::LIST);
    auto stuff = static_cast<const DefaultList*>(parsed.get());
    EXPECT_EQ(stuff->size(), 0);
}

TEST(JsonList, Nested) {
    auto parsed = load_json_strict("{ \"type\":\"list\", \"values\": [ { \"type\": \"nothing\" }, { \"type\": \"list\", \"values\": [ { \"type\": \"nothing\" } ] } ] }");
    EXPECT_EQ(parsed->type(), uzuki2::LIST);

    auto stuff = static_cast<const DefaultList*>(parsed.get());
    EXPECT_EQ(stuff->size(), 2);

    EXPECT_EQ(stuff->values[0]->type(), uzuki2::NOTHING);
    EXPECT_EQ(stuff->values[1]->type(), uzuki2::LIST);

    auto lptr = static_cast<const DefaultList*>(stuff->values[1].get());
    EXPECT_EQ(lptr->size(), 1);
    EXPECT_EQ(lptr->values[0]->type(), uzuki2::NOTHING);
}

TEST(JsonList, CheckError) {
    expect_json_error("{ \"type\":\"list\" }", "expected 'values' property");
    expect_json_error("{ \"type\":\"list\", \"values\": 1 }", "expected an array");
    expect_json_error("{ \"type\":\"list\", \"values\": [true] }", "should be represented by a JSON object");
    expect_json_error("{ \"type\":\"list\", \"values\": [ { \"type\": \"nothing\" } ], \"names\": [\"X\", \"Y\"] }", "should be the same");
}

