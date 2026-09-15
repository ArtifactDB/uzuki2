#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <vector>

#include "uzuki2/parse_json.hpp"

#include "utils.h"
#include "../test_subclass.h"

TEST(JsonBoolean, Vector) {
    auto parsed = load_json("{ \"type\": \"boolean\", \"values\": [ true, false, false, true ] }");
    EXPECT_EQ(parsed->type(), uzuki2::BOOLEAN);
    auto bptr = static_cast<const DefaultBooleanVector*>(parsed.get());
    EXPECT_EQ(bptr->size(), 4);
    EXPECT_FALSE(bptr->base.scalar);

    std::vector<std::uint8_t> expected { 1, 0, 0, 1 };
    EXPECT_EQ(bptr->base.values, expected);
}

TEST(JsonBoolean, Scalar) {
    auto parsed = load_json("{ \"type\": \"boolean\", \"values\": true }");
    EXPECT_EQ(parsed->type(), uzuki2::BOOLEAN);
    auto stuff = static_cast<const DefaultBooleanVector*>(parsed.get());
    EXPECT_TRUE(stuff->base.scalar);
    EXPECT_TRUE(stuff->base.values[0]);
}

TEST(JsonBoolean, MissingValues) {
    auto parsed = load_json("{ \"type\": \"boolean\", \"values\": [ true, null, false ] }");
    EXPECT_EQ(parsed->type(), uzuki2::BOOLEAN);
    auto bptr = static_cast<const DefaultBooleanVector*>(parsed.get());
    std::vector<std::uint8_t> expected { 1, 255, 0 };
    EXPECT_EQ(bptr->base.values, expected);
}

TEST(JsonBoolean, Error) {
    expect_json_error("{\"type\":\"boolean\", \"values\":[true,1,2] }", "expected a boolean");
}
