#ifndef UZUKI2_PARSE_HPP
#define UZUKI2_PARSE_HPP

#include <memory>
#include <vector>
#include <cctype>
#include <string>
#include <stdexcept>
#include <unordered_map>

#include "interfaces.hpp"
#include "Dummy.hpp"

namespace uzuki2 {

/**
 * @cond
 */
namespace json {

enum JsonType {
    NUMBER,
    STRING,
    BOOLEAN,
    NOTHING,
    ARRAY,
    OBJECT
};

struct Base {
    virtual JsonType type() const = 0;
};

struct Number : public Base {
    Number(double v) : value(v) {}
    double value;
    JsonType type() const { return NUMBER; }
};

struct String : public Base {
    String(std::string s) : value(std::move(s)) {}
    std::string value;
    JsonType type() const { return STRING; }
};

struct Boolean : public Base {
    Boolean(bool v) : value(v) {}
    bool value;
    JsonType type() const { return BOOLEAN; }
};

struct Null : public Base {
    JsonType type() const { return NOTHING; }
};

struct Array : public Base {
    std::vector<std::shared_ptr<Base> > values;
    JsonType type() const { return ARRAY; }
};

struct Object {
    std::unordered_map<std::string, std::shared_ptr<Base> > values;
    JsonType type() const { return OBJECT; }
};

struct CurrentBuffer {
    const char* ptr = nullptr;
    size_t available = 0;
    size_t position = 0;
    bool remaining = false;
    size_t overall = 0;

    std::string locstr() const {
        return std::to_string(overall + position + 1);
    }

    bool valid() const {
        return position < available || remaining;
    }

    void advance() {
        ++position;
    }

    template<class Reader>
    char get(Reader& reader) {
        if (position == available) {
            overall += available;
            remaining = reader();
            ptr = reinterpret_cast<const char*>(reader.buffer());
            available = reader.available();
            position = 0;

            if (available == 0) {
                throw std::runtime_error("no available bytes left");
            }
        }

        return ptr[position];
    }
};

template<class Reader>
void chomp(CurrentBuffer& buffer, Reader& reader) {
    while (buffer.valid() && std::isspace(buffer.get(reader))) {
        buffer.advance();
    }
    return;
}

template<class Reader>
bool is_expected_string(CurrentBuffer& buffer, Reader& reader, const std::string& expected) {
    for (auto x : expected) {
        if (!buffer.valid()) {
            return false;
        }
        if (buffer.get(reader) != x) {
            return false;
        }
        buffer.advance();
    }
    return true;
}

template<class Reader>
std::string extract_string(CurrentBuffer& buffer, Reader& reader) {
    size_t start = i + buffer.overall;
    buffer.advance(); // get past the opening quote.
    std::string output;

    while (1) {
        char next = buffer.get(reader);
        switch (next) {
            case '"':
                // Auto-increment in get() will automatically get past the trailing quote.
                return output;
            case '\\':
                if (!buffer.valid()) {
                    throw std::runtime_error("unterminated string at position " + std::to_string(start + 1));
                }
                switch (buffer.get(reader)) {
                    case '"':
                        output += '"';          
                        break;
                    case 'n':
                        output += '\n';
                        break;
                    case 'r':
                        output += '\r';
                        break;
                    case '\\':
                        output += '\\';
                        break;
                    case 'b':
                        output += '\b';
                        break;
                    case 'f':
                        output += '\f';
                        break;
                    case 't':
                        output += '\t';
                        break;
                    // TODO: \u unicode?
                    default:
                        throw std::runtime_error("unrecognized escape '\\" + buffer.ptr[i] + "'");
                }
                break;
            default:
                output += next;
                break;
        }

        buffer.advance();
        if (!buffer.valid()) {
            throw std::runtime_error("unterminated string at position " + std::to_string(start + 1));
        }
    }

    return output; // Technically unreachable, but whatever.
}

template<class Reader>
double extract_number(CurrentBuffer& buffer, Reader& reader) {
    size_t start = i + buffer.overall;
    double value = 0;
    double fractional = 10;
    double exponent = 0; 
    bool negative_exponent = false;

    auto is_terminator = [](char v) -> bool {
        return v == ',' || v == ']' || v == '}' || std::isspace(v);
    };

    auto finalizer = [&]() -> double {
        if (exponent) {
            if (negative_exponent) {
                exponent *= -1;
            }
            return value * std::pow(10.0, exponent);
        } else {
            return value;
        }
    };

    bool in_fraction = false;
    bool in_exponent = false;

    // We assume we're starting from the absolute value, after removing any preceding negative sign. 
    char lead = buffer.get(reader);
    if (lead == '0') {
        buffer.advance();
        if (!buffer.valid()) {
            return 0;
        }

        char val = buffer.get(reader);
        if (val == '.') {
            in_fraction = true;
        } else if (val == 'e' || val == 'E') {
            in_exponent = true;
        } else if (is_terminator(val)) {
            return finalizer();
        } else {
            throw std::runtime_error("invalid number starting with 0 at position " + std::to_string(start + 1));
        }

    } else if (std::isdigit(lead)) {
        value += lead - '0';
        buffer.advance();

        while (buffer.valid()) {
            char val = buffer.get(reader);
            if (val == '.') {
                in_fraction = true;
                break;
            } else if (val == 'e' || val == 'E') {
                in_exponent = true;
                break;
            } else if (is_terminator(val)) {
                return finalizer();
            } else if (!std::isdigit(val)) {
                throw std::runtime_error("invalid number containing '" + val + "' at position " + std::to_string(start + 1));
            }
            value *= 10;
            value += val - '0';
            buffer.advance();
        }

    } else {
        // this should never happen, as extract_number is only called when the lead is a digit (or '-').
    }

    if (in_fraction) {
        buffer.advance();
        if (!buffer.valid()) {
            throw std::runtime_error("invalid number with trailing '.' at position " + std::to_string(start + 1));
        }

        char val = buffer.get(reader);
        if (!std::isdigit(val)) {
            throw std::runtime_error("'.' must be followed by at least one digit at position " + std::to_string(start + 1));
        }
        value += (val - '0') / fractional;

        buffer.advance();
        while (buffer.valid()) {
            char val = buffer.get(reader);
            if (val == 'e' || val == 'E') {
                in_exponent = true;
                break;
            } else if (is_terminator(val)) {
                return finalizer();
            } else if (!std::isdigit(val)) {
                throw std::runtime_error("invalid number containing '" + val + "' at position " + std::to_string(start + 1));
            }
            fractional *= 10;
            value += (val - '0') / fractional;
            buffer.advance();
        } 
    }

    if (in_exponent) {
        buffer.advance();
        if (!buffer.valid()) {
            throw std::runtime_error("invalid number with trailing 'e/E' at position " + std::to_string(start + 1));
        }

        char val = buffer.get(reader);
        if (val == '+') {
            ;
        } else if (val == '-') {
            negative_exponent = true;
        } else {
            throw std::runtime_error("'e/E' must be followed by at least one digit at position " + std::to_string(start + 1));
        }

        buffer.advance();
        if (!buffer.valid()) {
            throw std::runtime_error("invalid number with trailing exponent sign at position " + std::to_string(start + 1));
        }
        val = buffer.get(reader);
        if (!std::isdigit(val)) {
            throw std::runtime_error("exponent sign must be followed by at least one digit at position " + std::to_string(start + 1));
        }
        exponent += (val - '0');

        buffer.advance();
        while (buffer.valid()) {
            char val = buffer.get(reader);
            if (is_terminator(val)) {
                return finalizer();
            } else if (!std::isdigit(val)) {
                throw std::runtime_error("invalid number containing '" + val + "' at position " + std::to_string(start + 1));
            }
            exponent *= 10;
            exponent += (val - '0');
            buffer.advance();
        } 
    }

    return finalizer();
}

template<class Reader>
std::shared_ptr<Base> parse_thing(CurrentBuffer& buffer, Reader& reader) {
    std::shared_ptr<Base> output;

    size_t start = buffer.position + buffer.overall + 1;
    const char current = buffer.get(reader);

    if (current == 't') {
        if (!is_expected_string(buffer, reader, "true")) {
            throw std::runtime_error("expected a 'true' string at position " + std::to_string(start));
        }
        output.reset(new Boolean(true));

    } else if (current == 'f') {
        if (!is_expected_string(buffer, reader, "false")) {
            throw std::runtime_error("expected a 'false' string at position " + std::to_string(start));
        }
        output.reset(new Boolean(false));

    } else if (current == 'n') {
        if (!is_expected_string(buffer, reader, "null")) {
            throw std::runtime_error("expected a 'null' string at position " + std::to_string(start));
        }
        output.reset(new Nothing);

    } else if (current == '"') {
        output.reset(new String(extract_string(buffer, reader)));

    } else if (current == '[') {
        auto ptr = new Array;
        output.reset(ptr);

        buffer.advance();
        bool okay = false;
        while (1) {
            chomp(buffer, reader);
            if (!buffer.valid()) {
                throw std::runtime_error("unterminated array starting at position " + std::to_string(start));
            }
            ptr->values.push_back(parse_thing(buffer, reader));

            chomp(buffer, reader);
            if (!buffer.valid()) {
                throw std::runtime_error("unterminated array starting at position " + std::to_string(start));
            }

            char next = buffer.get(reader);
            buffer.advance(); // make sure we skip the closing brace before returning.
            if (next == ']') {
                break;
            } else if (next != ',') {
                throw std::runtime_error("unknown character '" + std::string(next) + "' in array at position " + std::to_string(buffer.position + buffer.overall + 1));
            }
        }

    } else if (current == '{') {
        auto ptr = new Object;
        output.reset(ptr);
        auto& map = ptr->values;

        buffer.advance();
        while (1) {
            chomp(buffer, reader);
            if (!buffer.valid()) {
                throw std::runtime_error("unterminated object starting at position " + std::to_string(start));
            }

            char next = buffer.get(reader);
            if (next != '"') {
                throw std::runtime_error("expected a string as the object key at position " + std::to_string(buffer.position + buffer.overall + 1));
            }
            auto key = parse_string(buffer, reader);
            if (map.find(key) != map.end()) {
                throw std::runtime_error("detected duplicate keys in the object at position " + std::to_string(buffer.position + buffer.overall + 1));
            }

            chomp(buffer, reader);
            if (!buffer.valid()) {
                throw std::runtime_error("unterminated object starting at position " + std::to_string(start));
            }
            if (buffer.get(reader) != ':') {
                throw std::runtime_error("expected ':' to separate keys and values at position " + std::to_string(buffer.position + buffer.overall + 1));
            }

            buffer.advance();
            chomp(buffer, reader);
            if (!buffer.valid()) {
                throw std::runtime_error("unterminated object starting at position " + std::to_string(start));
            }
            map[key] = parse_thing(buffer, reader);

            chomp(buffer, reader);
            if (!buffer.valid()) {
                throw std::runtime_error("unterminated object starting at position " + std::to_string(start));
            }

            char next = buffer.get(reader);
            buffer.advance(); // make sure we skip the closing brace before returning.
            if (next == '}') {
                break;
            } else if (next != ',') {
                throw std::runtime_error("unknown character '" + std::string(next) + "' in array at position " + std::to_string(buffer.position + buffer.overall + 1));
            }
        }

    } else if (current == '-') {
        buffer.advance(); 
        output.reset(new Number(-extract_number(buffer, reader);

    } else if (std::isdigit(current)) {
        output.reset(new Number(extract_number(buffer, reader);

    } else {
        throw std::runtime_error(std::string("unknown type starting with '") + std::string(current) + "' at position " + std::to_string(buffer.position + 1));
    }

    return output;
}

}
/**
 * @endcond
 */

}
