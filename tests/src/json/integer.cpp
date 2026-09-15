#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_json.hpp"

#include "utils.h"
#include "../test_subclass.h"

TEST(JsonInteger, Vector) {
    auto parsed = load_json("{ \"type\": \"integer\", \"values\": [ 0, 1000, -1, 12345, -2e+4 ] }");
    EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
    auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
    std::vector<std::int32_t> expected{ 0, 1000, -1, 12345, -20000 };
    EXPECT_FALSE(iptr->base.scalar);
    EXPECT_EQ(iptr->base.values, expected);
}

TEST(JsonInteger, Scalar) {
    auto parsed = load_json("{ \"type\": \"integer\", \"values\": 1234 }");
    EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
    auto stuff = static_cast<const DefaultIntegerVector*>(parsed.get());
    EXPECT_TRUE(stuff->base.scalar);
    EXPECT_EQ(stuff->base.values[0], 1234);
}

TEST(JsonInteger, LegacyMissing) {
    // -2^31 is respected as a missing placeholder value in legacy versions.
    auto parsed = load_json("{ \"type\": \"integer\", \"values\": [ 0, 1000, -2147483648, null, -2e+4 ] }");
    EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
    auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
    std::vector<std::int32_t> expected{ 0, 1000, -123456789, -123456789, -20000 }; // i.e., the missing value placeholder in our tests.
    EXPECT_EQ(iptr->base.values, expected);
}

TEST(JsonInteger, Missing) {
    // In more recent versions, -2^31 is no longer a special value.
    auto parsed = load_json("{ \"type\": \"integer\", \"values\": [ 0, 1000, -2147483648, null, -2e+4 ], \"version\":\"1.1\" }");
    EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
    auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
    std::vector<std::int32_t> expected{ 0, 1000, -2147483648, -123456789, -20000 }; // i.e., the missing value placeholder in our tests.
    EXPECT_EQ(iptr->base.values, expected);
}

TEST(JsonInteger, Error) {
    expect_json_error("{ \"type\": \"integer\" }", "expected 'values' property");
    expect_json_error("{ \"type\": \"integer\", \"values\": \"foo\"}", "expected a number");

    expect_json_error("{ \"type\": \"integer\", \"values\": [true]}", "expected a number");
    expect_json_error("{ \"type\": \"integer\", \"values\": [1.2]}", "expected an integer");
    expect_json_error("{ \"type\": \"integer\", \"values\": [-999999999999]}", "cannot be represented");
}
