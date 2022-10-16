#ifndef ERROR_H
#define ERROR_H

#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "uzuki2/validate.hpp"

inline void expect_error(std::string file, std::string name, std::string msg) {
    EXPECT_ANY_THROW({
        try {
            uzuki2::validate(file, name);
        } catch (std::exception& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
            throw;
        }
    });
}

#endif
