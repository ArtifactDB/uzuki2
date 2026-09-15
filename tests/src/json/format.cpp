#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_json.hpp"

#include "utils.h"
#include "../test_subclass.h"

TEST(JsonFormat, Error) {
    expect_json_error("{\"type\":\"string\", \"format\":2, \"values\":[\"foo\", \"bar\"], \"version\":\"1.1\"}", "expected a string");
    expect_json_error("{\"type\":\"string\", \"format\":\"whee\", \"values\":[\"foo\", \"bar\"], \"version\":\"1.1\"}", "unsupported format");
}
