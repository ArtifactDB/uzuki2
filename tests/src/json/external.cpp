#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstddef>

#include "uzuki2/parse_json.hpp"

#include "utils.h"
#include "../test_subclass.h"

auto load_json_with_externals(std::string x, int num_externals) {
    DefaultExternals ext(num_externals);
    uzuki2::json::Options opt;
    opt.strict_list = false;
    return uzuki2::json::parse_buffer<DefaultProvisioner>(reinterpret_cast<const unsigned char*>(x.c_str()), x.size(), ext, std::move(opt));
}

TEST(JsonExternal, Single) {
    auto parsed = load_json_with_externals("{ \"type\": \"external\", \"index\": 0 }", 1);
    EXPECT_EQ(parsed->type(), uzuki2::EXTERNAL);
    auto stuff = static_cast<const DefaultExternal*>(parsed.get());
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(stuff->ptr), 1);
}

TEST(JsonExternal, Multiple) {
    auto parsed = load_json_with_externals("{ \"type\": \"list\", \"values\": [ { \"type\": \"external\", \"index\": 1 }, { \"type\": \"external\", \"index\": 0 } ] }", 2);
    EXPECT_EQ(parsed->type(), uzuki2::LIST);
    auto list = static_cast<const DefaultList*>(parsed.get());

    auto stuff = static_cast<const DefaultExternal*>(list->values[0].get());
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(stuff->ptr), 2);

    auto stuff2 = static_cast<const DefaultExternal*>(list->values[1].get());
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(stuff2->ptr), 1);
}

void expect_json_external_error(std::string x, std::string msg, int num_expected) {
    std::string obs;
    try {
        uzuki2::json::validate_buffer(reinterpret_cast<const unsigned char*>(x.c_str()), x.size(), num_expected, {});
    } catch (std::exception& e) {
        obs = e.what();
    }
    EXPECT_THAT(obs, ::testing::HasSubstr(msg));
}

TEST(JsonExternal, CheckErrors) {
    expect_json_external_error("{\"type\":\"external\"}", "expected 'index'", 1);
    expect_json_external_error("{\"type\":\"external\", \"index\":false}", "expected a number", 1);
    expect_json_external_error("{\"type\":\"external\", \"index\":1.2}", "expected an integer", 1);
    expect_json_external_error("{\"type\":\"external\", \"index\":-1}", "non-negative", 1);
    expect_json_external_error("{\"type\":\"external\", \"index\":2}", "out of range", 1);
    expect_json_external_error("{ \"type\": \"list\", \"values\": [ { \"type\": \"external\", \"index\": 0 }, { \"type\": \"external\", \"index\": 0 } ] }", "consecutive", 2);
}
