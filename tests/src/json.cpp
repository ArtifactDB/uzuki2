#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_json.hpp"
#include "byteme/RawBufferReader.hpp"

#include "test_subclass.h"
#include "utils.h"
#include "error.h"

std::shared_ptr<uzuki2::json::Base> parse_json_string(std::string x) {
    auto ptr = reinterpret_cast<const unsigned char*>(x.c_str());
    byteme::RawBufferReader reader(ptr, x.size());
    return uzuki2::json::parse_json(reader);
}

void expect_json_error(std::string x, std::string msg) {
    EXPECT_ANY_THROW({
        try {
            parse_json_string(x);
        } catch (std::exception& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
            throw;
        }
    });
}

TEST(JsonParsingTest, NullLoading) {
    {
        auto output = parse_json_string("null");
        EXPECT_EQ(output->type(), uzuki2::json::NOTHING);
    }

    expect_json_error("none", "expected a 'null'");
    expect_json_error("nully", "trailing");
}

TEST(JsonParsingTest, BooleanLoading) {
    {
        auto output = parse_json_string("true");
        EXPECT_EQ(output->type(), uzuki2::json::BOOLEAN);
        EXPECT_TRUE(static_cast<uzuki2::json::Boolean*>(output.get())->value);
    }

    {
        auto output = parse_json_string("false");
        EXPECT_EQ(output->type(), uzuki2::json::BOOLEAN);
        EXPECT_FALSE(static_cast<uzuki2::json::Boolean*>(output.get())->value);
    }

    expect_json_error("falsy", "expected a 'false'");
    expect_json_error("falsey", "trailing");
    expect_json_error("truthy", "expected a 'true'");
    expect_json_error("true-ish", "trailing");
}

TEST(JsonParsingTest, StringLoading) {
    {
        auto output = parse_json_string("\t\n \"aaron was here\" ");
        EXPECT_EQ(output->type(), uzuki2::json::STRING);
        EXPECT_EQ(static_cast<uzuki2::json::String*>(output.get())->value, "aaron was here");
    }

    {
        auto output = parse_json_string("\"do\\\"you\\nbelieve\\tin\\rlife\\fafter\\blove\\\\\"");
        EXPECT_EQ(output->type(), uzuki2::json::STRING);
        EXPECT_EQ(static_cast<uzuki2::json::String*>(output.get())->value, "do\"you\nbelieve\tin\rlife\fafter\blove\\");
    }

    expect_json_error(" \"asdasdaasd ", "unterminated string");
    expect_json_error(" \"asdasdaasd\\", "unterminated string");
    expect_json_error(" \"asdasdaasd\\a", "unrecognized escape");
}

TEST(JsonParsingTest, IntegerLoading) {
    {
        auto output = parse_json_string(" 12345 ");
        EXPECT_EQ(output->type(), uzuki2::json::NUMBER);
        EXPECT_EQ(static_cast<uzuki2::json::Number*>(output.get())->value, 12345);
    }

    {
        auto output = parse_json_string(" 123"); // no trailing space.
        EXPECT_EQ(output->type(), uzuki2::json::NUMBER);
        EXPECT_EQ(static_cast<uzuki2::json::Number*>(output.get())->value, 123);
    }

    {
        auto output = parse_json_string(" 0 ");
        EXPECT_EQ(output->type(), uzuki2::json::NUMBER);
        EXPECT_EQ(static_cast<uzuki2::json::Number*>(output.get())->value, 0);
    }

    {
        auto output = parse_json_string(" 0"); // no trailing space.
        EXPECT_EQ(output->type(), uzuki2::json::NUMBER);
        EXPECT_EQ(static_cast<uzuki2::json::Number*>(output.get())->value, 0);
    }

    {
        auto output = parse_json_string(" -789 ");
        EXPECT_EQ(output->type(), uzuki2::json::NUMBER);
        EXPECT_EQ(static_cast<uzuki2::json::Number*>(output.get())->value, -789);
    }

    expect_json_error(" 1234L ", "containing 'L'");
    expect_json_error(" 0123456 ", "starting with 0");
    expect_json_error(" 1.", "trailing '.'");
    expect_json_error(" -", "incomplete number");
}

TEST(JsonParsingTest, FractionLoading) {
    {
        auto output = parse_json_string(" 1.2345 ");
        EXPECT_EQ(output->type(), uzuki2::json::NUMBER);
        EXPECT_EQ(static_cast<uzuki2::json::Number*>(output.get())->value, 1.2345);
    }

    {
        auto output = parse_json_string(" 123.456 ");
        EXPECT_EQ(output->type(), uzuki2::json::NUMBER);
        EXPECT_EQ(static_cast<uzuki2::json::Number*>(output.get())->value, 123.456);
    }

    {
        auto output = parse_json_string(" 512.00"); // no trailing space.
        EXPECT_EQ(output->type(), uzuki2::json::NUMBER);
        EXPECT_EQ(static_cast<uzuki2::json::Number*>(output.get())->value, 512);
    }

    {
        auto output = parse_json_string(" -0.123456 ");
        EXPECT_EQ(output->type(), uzuki2::json::NUMBER);
        EXPECT_FLOAT_EQ(static_cast<uzuki2::json::Number*>(output.get())->value, -0.123456);
    }

    expect_json_error(" 00.12345 ", "starting with 0");
    expect_json_error(" .12345 ", "starting with '.'");
    expect_json_error(" 12.34f ", "containing 'f'");
}

TEST(JsonParsingTest, ScientificLoading) {
    {
        auto output = parse_json_string(" 1e+2 ");
        EXPECT_EQ(output->type(), uzuki2::json::NUMBER);
        EXPECT_EQ(static_cast<uzuki2::json::Number*>(output.get())->value, 100);
    }

    {
        auto output = parse_json_string(" 1e+002 ");
        EXPECT_EQ(output->type(), uzuki2::json::NUMBER);
        EXPECT_EQ(static_cast<uzuki2::json::Number*>(output.get())->value, 100);
    }

    {
        auto output = parse_json_string(" 1e-2 ");
        EXPECT_EQ(output->type(), uzuki2::json::NUMBER);
        EXPECT_EQ(static_cast<uzuki2::json::Number*>(output.get())->value, 0.01);
    }

    {
        auto output = parse_json_string(" 123e-1"); // no trailing space.
        EXPECT_EQ(output->type(), uzuki2::json::NUMBER);
        EXPECT_EQ(static_cast<uzuki2::json::Number*>(output.get())->value, 12.3);
    }

    {
        auto output = parse_json_string(" 1.918E+2 ");
        EXPECT_EQ(output->type(), uzuki2::json::NUMBER);
        EXPECT_FLOAT_EQ(static_cast<uzuki2::json::Number*>(output.get())->value, 191.8);
    }

    expect_json_error(" 1e", "trailing 'e/E'");
    expect_json_error(" 1e ", "'e/E' must be followed");
    expect_json_error(" 1e+", "trailing exponent sign");
    expect_json_error(" 1e+ ", "must be followed by at least one digit");
    expect_json_error(" 1e+1a", "containing 'a'");
}

TEST(JsonParsingTest, ArrayLoading) {
    {
        // Check that numbers are correctly terminated by array delimiters.
        auto output = parse_json_string("[100, 200.00, 3.00e+2]");
        EXPECT_EQ(output->type(), uzuki2::json::ARRAY);
        auto aptr = static_cast<uzuki2::json::Array*>(output.get());
        EXPECT_EQ(aptr->values.size(), 3);

        EXPECT_EQ(aptr->values[0]->type(), uzuki2::json::NUMBER);
        EXPECT_EQ(static_cast<uzuki2::json::Number*>(aptr->values[0].get())->value, 100);

        EXPECT_EQ(aptr->values[1]->type(), uzuki2::json::NUMBER);
        EXPECT_EQ(static_cast<uzuki2::json::Number*>(aptr->values[1].get())->value, 200);

        EXPECT_EQ(aptr->values[2]->type(), uzuki2::json::NUMBER);
        EXPECT_EQ(static_cast<uzuki2::json::Number*>(aptr->values[2].get())->value, 300);
    }

    {
        // Throwing some spaces between the structural elements.
        auto output = parse_json_string("[ true , false , null , \"[true, false, null]\" ]");
        EXPECT_EQ(output->type(), uzuki2::json::ARRAY);
        auto aptr = static_cast<uzuki2::json::Array*>(output.get());
        EXPECT_EQ(aptr->values.size(), 4);

        EXPECT_EQ(aptr->values[0]->type(), uzuki2::json::BOOLEAN);
        EXPECT_TRUE(static_cast<uzuki2::json::Boolean*>(aptr->values[0].get())->value);

        EXPECT_EQ(aptr->values[1]->type(), uzuki2::json::BOOLEAN);
        EXPECT_FALSE(static_cast<uzuki2::json::Boolean*>(aptr->values[1].get())->value);

        EXPECT_EQ(aptr->values[2]->type(), uzuki2::json::NOTHING);

        EXPECT_EQ(aptr->values[3]->type(), uzuki2::json::STRING);
        EXPECT_EQ(static_cast<uzuki2::json::String*>(aptr->values[3].get())->value, "[true, false, null]");
    }

    {
        // No spaces at all.
        auto output = parse_json_string("[null,false,true]");
        EXPECT_EQ(output->type(), uzuki2::json::ARRAY);
        auto aptr = static_cast<uzuki2::json::Array*>(output.get());
        EXPECT_EQ(aptr->values.size(), 3);

        EXPECT_EQ(aptr->values[0]->type(), uzuki2::json::NOTHING);

        EXPECT_EQ(aptr->values[1]->type(), uzuki2::json::BOOLEAN);
        EXPECT_FALSE(static_cast<uzuki2::json::Boolean*>(aptr->values[1].get())->value);

        EXPECT_EQ(aptr->values[2]->type(), uzuki2::json::BOOLEAN);
        EXPECT_TRUE(static_cast<uzuki2::json::Boolean*>(aptr->values[2].get())->value);
    }

    {
        // Empty array.
        auto output = parse_json_string("[]");
        EXPECT_EQ(output->type(), uzuki2::json::ARRAY);
        auto aptr = static_cast<uzuki2::json::Array*>(output.get());
        EXPECT_EQ(aptr->values.size(), 0);

        output = parse_json_string("[   ]");
        EXPECT_EQ(output->type(), uzuki2::json::ARRAY);
        aptr = static_cast<uzuki2::json::Array*>(output.get());
        EXPECT_EQ(aptr->values.size(), 0);
    }

    expect_json_error(" [", "unterminated array");
    expect_json_error(" [ 1,", "unterminated array");
    expect_json_error(" [ 1, ", "unterminated array");
    expect_json_error(" [ 1, ]", "unknown type starting with ']'");
    expect_json_error(" [ 1 1 ]", "unknown character '1'");
    expect_json_error(" [ , ]", "unknown type starting with ','");
}

TEST(JsonParsingTest, ObjectLoading) {
    {
        // Check that numbers are correctly terminated by object delimiters.
        auto output = parse_json_string("{\"foo\": 1, \"bar\":2, \"whee\":3}");
        EXPECT_EQ(output->type(), uzuki2::json::OBJECT);
        auto aptr = static_cast<uzuki2::json::Object*>(output.get());
        EXPECT_EQ(aptr->values.size(), 3);

        const auto& foo = aptr->values["foo"];
        EXPECT_EQ(foo->type(), uzuki2::json::NUMBER);
        EXPECT_EQ(static_cast<uzuki2::json::Number*>(foo.get())->value, 1);

        const auto& bar = aptr->values["bar"];
        EXPECT_EQ(bar->type(), uzuki2::json::NUMBER);
        EXPECT_EQ(static_cast<uzuki2::json::Number*>(bar.get())->value, 2);

        const auto& whee = aptr->values["whee"];
        EXPECT_EQ(whee->type(), uzuki2::json::NUMBER);
        EXPECT_EQ(static_cast<uzuki2::json::Number*>(whee.get())->value, 3);
    }

    {
        // Check that we're robust to spaces.
        auto output = parse_json_string("{ \"foo\" :true , \"bar\": false, \"whee\" : null }");
        EXPECT_EQ(output->type(), uzuki2::json::OBJECT);
        auto aptr = static_cast<uzuki2::json::Object*>(output.get());
        EXPECT_EQ(aptr->values.size(), 3);

        const auto& foo = aptr->values["foo"];
        EXPECT_EQ(foo->type(), uzuki2::json::BOOLEAN);
        EXPECT_TRUE(static_cast<uzuki2::json::Boolean*>(foo.get())->value);

        const auto& bar = aptr->values["bar"];
        EXPECT_EQ(bar->type(), uzuki2::json::BOOLEAN);
        EXPECT_FALSE(static_cast<uzuki2::json::Boolean*>(bar.get())->value);

        const auto& whee = aptr->values["whee"];
        EXPECT_EQ(whee->type(), uzuki2::json::NOTHING);
    }

    {
        // No spaces at all.
        auto output = parse_json_string("{\"aaron\":\"lun\",\"jayaram\":\"kancherla\"}");
        EXPECT_EQ(output->type(), uzuki2::json::OBJECT);
        auto aptr = static_cast<uzuki2::json::Object*>(output.get());
        EXPECT_EQ(aptr->values.size(), 2);

        const auto& foo = aptr->values["aaron"];
        EXPECT_EQ(foo->type(), uzuki2::json::STRING);
        EXPECT_EQ(static_cast<uzuki2::json::String*>(foo.get())->value, "lun");

        const auto& bar = aptr->values["jayaram"];
        EXPECT_EQ(bar->type(), uzuki2::json::STRING);
        EXPECT_EQ(static_cast<uzuki2::json::String*>(bar.get())->value, "kancherla");
    }

    {
        // Empty object.
        auto output = parse_json_string("{ }");
        EXPECT_EQ(output->type(), uzuki2::json::OBJECT);
        auto aptr = static_cast<uzuki2::json::Object*>(output.get());
        EXPECT_EQ(aptr->values.size(), 0);

        output = parse_json_string("{}");
        EXPECT_EQ(output->type(), uzuki2::json::OBJECT);
        aptr = static_cast<uzuki2::json::Object*>(output.get());
        EXPECT_EQ(aptr->values.size(), 0);
    }

    expect_json_error(" {", "unterminated object");
    expect_json_error(" { \"foo\"", "unterminated object");
    expect_json_error(" { \"foo\" :", "unterminated object");
    expect_json_error(" { \"foo\" : \"bar\"", "unterminated object");
    expect_json_error(" { true", "expected a string");
    expect_json_error(" { \"foo\" , \"bar\" }", "expected ':'");
    expect_json_error(" { \"foo\": \"bar\", }", "expected a string");
    expect_json_error(" { \"foo\": \"bar\": \"stuff\" }", "unknown character ':'");
}
