#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_json.hpp"

#include "ritsuko/ritsuko.hpp"

#include "utils.h"
#include "../test_subclass.h"

TEST(JsonNumber, Vector) {
    auto parsed = load_json("{\"type\":\"number\", \"values\":[1.2, -3.5, -0.2, 1.343e+2] }");
    EXPECT_EQ(parsed->type(), uzuki2::NUMBER);
    auto bptr = static_cast<const DefaultNumberVector*>(parsed.get());
    EXPECT_EQ(bptr->size(), 4);
    EXPECT_FALSE(bptr->base.scalar);
    EXPECT_EQ(bptr->base.values[0], 1.2);
    EXPECT_EQ(bptr->base.values[1], -3.5);
    EXPECT_EQ(bptr->base.values[2], -0.2);
    EXPECT_EQ(bptr->base.values[3], 134.3);
}

TEST(JsonNumber, Scalar) {
    auto parsed = load_json("{ \"type\": \"number\", \"values\": 12.34 }");
    EXPECT_EQ(parsed->type(), uzuki2::NUMBER);
    auto stuff = static_cast<const DefaultNumberVector*>(parsed.get());
    EXPECT_TRUE(stuff->base.scalar);
    EXPECT_EQ(stuff->base.values[0], 12.34);
}

TEST(JsonNumber, Special) {
    auto parsed = load_json("{\"type\":\"number\", \"values\":[1.2, null, \"Inf\", \"-Inf\", \"NaN\"] }");
    EXPECT_EQ(parsed->type(), uzuki2::NUMBER);

    auto bptr = static_cast<const DefaultNumberVector*>(parsed.get());
    EXPECT_EQ(bptr->size(), 5);
    EXPECT_EQ(bptr->base.values[1], -123456789);
    EXPECT_TRUE(std::isinf(bptr->base.values[2]));
    EXPECT_TRUE(bptr->base.values[2] > 0);
    EXPECT_TRUE(std::isinf(bptr->base.values[3]));
    EXPECT_TRUE(bptr->base.values[3] < 0);
    EXPECT_TRUE(std::isnan(bptr->base.values[4]));
}

TEST(JsonNumberTest, Error) {
    expect_json_error("{ \"type\": \"number\", \"values\": [true]}", "expected a number");
    expect_json_error("{ \"type\": \"number\", \"values\": [\"nan\"]}", "unsupported string");
}
