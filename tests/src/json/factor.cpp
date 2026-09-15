#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_json.hpp"

#include "utils.h"
#include "../test_subclass.h"

TEST(JsonFactor, Simple) {
    std::vector<std::string> levels{ "akari", "alice", "aika" };
    std::vector<std::int32_t> codes{ 0, 1, 1, 0, 2 };
    auto parsed = load_json("{ \"type\": \"factor\", \"values\": [ " + arrayify(codes) + "], \"levels\": [" + arrayify(levels) + "] }");
    EXPECT_EQ(parsed->type(), uzuki2::FACTOR);

    auto fptr = static_cast<const DefaultFactor*>(parsed.get());
    EXPECT_EQ(fptr->vbase.values, codes);
    EXPECT_EQ(fptr->levels, levels);
    EXPECT_FALSE(fptr->ordered);
}

TEST(JsonFactor, LegacyOrdered) {
    std::vector<std::string> levels{ "athena", "akira", "alicia" };
    std::vector<std::int32_t> codes{ 2, 1, 0, 1, 2, 1, 0 };
    auto parsed = load_json("{ \"type\": \"ordered\", \"values\": [ " + arrayify(codes) + " ], \"levels\": [ " + arrayify(levels) + "] }");
    EXPECT_EQ(parsed->type(), uzuki2::FACTOR);

    auto fptr = static_cast<const DefaultFactor*>(parsed.get());
    EXPECT_TRUE(fptr->ordered);
    EXPECT_EQ(fptr->vbase.values, codes);
    EXPECT_EQ(fptr->levels, levels);
}

TEST(JsonFactor, Ordered) {
    std::vector<std::string> levels{ "athena", "akira", "alicia" };
    std::vector<std::int32_t> codes{ 2, 1, 0, 1, 2, 1, 0 };

    {
        auto parsed = load_json(
            "{ \"type\": \"factor\", \"values\": [ " + 
            arrayify(codes) +
            " ], \"levels\": [ " +
            arrayify(levels) +
            "], \"ordered\": true, \"version\": \"1.1\" }"
        );
        EXPECT_EQ(parsed->type(), uzuki2::FACTOR);

        auto fptr = static_cast<const DefaultFactor*>(parsed.get());
        EXPECT_TRUE(fptr->ordered);
        EXPECT_EQ(fptr->vbase.values, codes);
        EXPECT_EQ(fptr->levels, levels);
    }

    // Responds to the negative case.
    {
        auto parsed = load_json(
            "{ \"type\": \"factor\", \"values\": [ " + 
            arrayify(codes) +
            " ], \"levels\": [ " +
            arrayify(levels) +
            "], \"ordered\": false, \"version\": \"1.1\" }"
        );
        EXPECT_EQ(parsed->type(), uzuki2::FACTOR);

        auto fptr = static_cast<const DefaultFactor*>(parsed.get());
        EXPECT_FALSE(fptr->ordered);
        EXPECT_EQ(fptr->vbase.values, codes);
        EXPECT_EQ(fptr->levels, levels);
    }
}

TEST(JsonFactor, LegacyMissing) {
    std::vector<std::string> levels{ "ai", "akari", "alicia" };

    auto parsed = load_json("{ \"type\": \"ordered\", \"values\": [ 2, 1, -2147483648, 0, null ], \"levels\": [ " + arrayify(levels) + " ] }");
    EXPECT_EQ(parsed->type(), uzuki2::FACTOR);

    auto fptr = static_cast<const DefaultFactor*>(parsed.get());
    std::vector<std::int32_t> expected{ 2, 1, -123456789, 0, -123456789 }; // i.e., the test's missing value placeholder.
    EXPECT_EQ(fptr->vbase.values, expected);
    EXPECT_EQ(fptr->levels, levels);

    // Special value doesn't work in the latest version.
   expect_json_error("{ \"type\": \"factor\", \"values\": [ 2, 1, -2147483648, 0, null ], \"levels\": [ " + arrayify(levels) + " ], \"version\":\"1.1\" }", "out of range");
}

TEST(JsonFactor, Missing) {
    std::vector<std::string> levels{ "ai", "akari", "alicia" };

    auto parsed = load_json("{ \"type\": \"factor\", \"values\": [ 2, 1, null, 0, null ], \"levels\": [ " + arrayify(levels) + " ], \"version\": \"1.1\" }");
    EXPECT_EQ(parsed->type(), uzuki2::FACTOR);

    auto fptr = static_cast<const DefaultFactor*>(parsed.get());
    std::vector<std::int32_t> expected{ 2, 1, -123456789, 0, -123456789 }; // i.e., the test's missing value placeholder.
    EXPECT_EQ(fptr->vbase.values, expected);
    EXPECT_EQ(fptr->levels, levels);
}

TEST(JsonFactor, Error) {
    auto levels_str = arrayify<std::string>({ "athena", "akira", "alicia" });

    expect_json_error("{ \"type\": \"ordered\", \"values\": [ true, 0 ], \"levels\": [ " + levels_str + " ] }", "expected a number");
    expect_json_error("{ \"type\": \"ordered\", \"values\": [ 1.2, 0 ], \"levels\": [ " + levels_str + " ] }", "expected an integer");

    expect_json_error("{ \"type\": \"ordered\", \"values\": [ 2, 1, 3, 0 ], \"levels\": [ " + levels_str + " ] }", "out of range");
    expect_json_error("{ \"type\": \"ordered\", \"values\": [ 2, 1, -1, 0 ], \"levels\": [ " + levels_str + " ] }", "out of range");
    expect_json_error("{ \"type\": \"ordered\", \"values\": [ 2, 1, 0 ], \"levels\": [ \"aria\", \"aria\", \"aria\" ] }", "duplicate string");

    expect_json_error("{ \"type\": \"factor\", \"values\": [ 1, 0 ], \"levels\": [ " + levels_str + " ], \"ordered\": 1, \"version\": \"1.1\" }", "expected a boolean");
    expect_json_error("{ \"type\": \"ordered\", \"values\": [ 1, 0 ], \"levels\": [ " + levels_str + " ], \"version\": \"1.1\" }", "unknown object type 'ordered'");
}
