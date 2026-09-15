#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <type_traits>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "H5Cpp.h"

#include "uzuki2/uzuki2.hpp"

#include "../test_subclass.h"

template<typename Input_>
std::string arrayify(const std::vector<Input_>& array) {
    std::string output;
    bool init = true;
    for (const auto& val : array) {
        if (!init) {
            output += ", ";
        } else {
            init = false;
        }
        if constexpr(std::is_same<std::string, Input_>::value) {
            output += "\"" + val + "\"";
        } else {
            output += std::to_string(val);
        }
    }
    return output;
}

inline auto load_json(std::string x, bool parallel = false) {
    uzuki2::json::Options opt;
    opt.parallel = parallel;
    opt.strict_list = false;
    return uzuki2::json::parse_buffer<DefaultProvisioner>(reinterpret_cast<const unsigned char*>(x.c_str()), x.size(), uzuki2::DummyExternals(), std::move(opt));
}

inline auto load_json_strict(std::string x, bool parallel = false) {
    uzuki2::json::Options opt;
    opt.parallel = parallel;
    return uzuki2::json::parse_buffer<DefaultProvisioner>(reinterpret_cast<const unsigned char*>(x.c_str()), x.size(), uzuki2::DummyExternals(), std::move(opt));
}

inline void expect_json_error(std::string json, std::string msg) {
    std::string obs;
    try {
        uzuki2::json::validate_buffer(reinterpret_cast<const unsigned char*>(json.c_str()), json.size(), 0, {});
    } catch (std::exception& e) {
        obs = e.what();
    }
    EXPECT_THAT(obs, ::testing::HasSubstr(msg));
}

#endif
