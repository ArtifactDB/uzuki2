#ifndef UZUKI2_PARSE_JSON_HPP
#define UZUKI2_PARSE_JSON_HPP

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

struct Nothing : public Base {
    JsonType type() const { return NOTHING; }
};

struct Array : public Base {
    std::vector<std::shared_ptr<Base> > values;
    JsonType type() const { return ARRAY; }
};

struct Object : public Base {
    std::unordered_map<std::string, std::shared_ptr<Base> > values;
    JsonType type() const { return OBJECT; }
};

struct CurrentBuffer {
private:
    const char* ptr = nullptr;
    size_t available = 0;
    size_t current = 0;
    bool remaining = false;
    size_t overall = 0;

    template<class Reader>
    void fill(Reader& reader) {
        remaining = reader();
        ptr = reinterpret_cast<const char*>(reader.buffer());
        available = reader.available();
        current = 0;
        if (available == 0) {
            throw std::runtime_error("no available bytes left");
        }
    }
public:
    template<class Reader>
    CurrentBuffer(Reader& reader) {
        fill(reader);
    }

    bool valid() const {
        return current < available || remaining;
    }

    void advance() {
        ++current;
    }

    template<class Reader>
    char get(Reader& reader) {
        if (current == available) {
            overall += available;
            fill(reader);
        }
        return ptr[current];
    }

    size_t position() {
        return overall + current;
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
    size_t start = buffer.position() + 1;
    buffer.advance(); // get past the opening quote.
    std::string output;

    while (1) {
        char next = buffer.get(reader);
        switch (next) {
            case '"':
                buffer.advance(); // get past the closing quote.
                return output;
            case '\\':
                buffer.advance();
                if (!buffer.valid()) {
                    throw std::runtime_error("unterminated string at position " + std::to_string(start));
                } else {
                    char next2 = buffer.get(reader);
                    switch (next2) {
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
                            throw std::runtime_error("unrecognized escape '\\" + std::string(1, next2) + "'");
                    }
                }
                break;
            default:
                output += next;
                break;
        }

        buffer.advance();
        if (!buffer.valid()) {
            throw std::runtime_error("unterminated string at position " + std::to_string(start));
        }
    }

    return output; // Technically unreachable, but whatever.
}

template<class Reader>
double extract_number(CurrentBuffer& buffer, Reader& reader) {
    size_t start = buffer.position() + 1;
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
            throw std::runtime_error("invalid number starting with 0 at position " + std::to_string(start));
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
                throw std::runtime_error("invalid number containing '" + std::string(1, val) + "' at position " + std::to_string(start));
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
            throw std::runtime_error("invalid number with trailing '.' at position " + std::to_string(start));
        }

        char val = buffer.get(reader);
        if (!std::isdigit(val)) {
            throw std::runtime_error("'.' must be followed by at least one digit at position " + std::to_string(start));
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
                throw std::runtime_error("invalid number containing '" + std::string(1, val) + "' at position " + std::to_string(start));
            }
            fractional *= 10;
            value += (val - '0') / fractional;
            buffer.advance();
        } 
    }

    if (in_exponent) {
        buffer.advance();
        if (!buffer.valid()) {
            throw std::runtime_error("invalid number with trailing 'e/E' at position " + std::to_string(start));
        }

        char val = buffer.get(reader);
        if (val == '+') {
            ;
        } else if (val == '-') {
            negative_exponent = true;
        } else {
            throw std::runtime_error("'e/E' must be followed by at least one digit in number at position " + std::to_string(start));
        }

        buffer.advance();
        if (!buffer.valid()) {
            throw std::runtime_error("invalid number with trailing exponent sign in number at position " + std::to_string(start));
        }
        val = buffer.get(reader);
        if (!std::isdigit(val)) {
            throw std::runtime_error("exponent sign must be followed by at least one digit in number at position " + std::to_string(start));
        }
        exponent += (val - '0');

        buffer.advance();
        while (buffer.valid()) {
            char val = buffer.get(reader);
            if (is_terminator(val)) {
                return finalizer();
            } else if (!std::isdigit(val)) {
                throw std::runtime_error("invalid number containing '" + std::string(1, val) + "' at position " + std::to_string(start));
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

    size_t start = buffer.position() + 1;
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
        chomp(buffer, reader);
        if (!buffer.valid()) {
            throw std::runtime_error("unterminated array starting at position " + std::to_string(start));
        }

        if (buffer.get(reader) != ']') {
            while (1) {
                ptr->values.push_back(parse_thing(buffer, reader));

                chomp(buffer, reader);
                if (!buffer.valid()) {
                    throw std::runtime_error("unterminated array starting at position " + std::to_string(start));
                }

                char next = buffer.get(reader);
                if (next == ']') {
                    break;
                } else if (next != ',') {
                    throw std::runtime_error("unknown character '" + std::string(1, next) + "' in array at position " + std::to_string(buffer.position() + 1));
                }

                buffer.advance(); 
                chomp(buffer, reader);
                if (!buffer.valid()) {
                    throw std::runtime_error("unterminated array starting at position " + std::to_string(start));
                }
            }
        }

        buffer.advance(); // skip the closing bracket.

    } else if (current == '{') {
        auto ptr = new Object;
        output.reset(ptr);
        auto& map = ptr->values;

        buffer.advance();
        chomp(buffer, reader);
        if (!buffer.valid()) {
            throw std::runtime_error("unterminated object starting at position " + std::to_string(start));
        }

        if (buffer.get(reader) != '}') {
            while (1) {
                char next = buffer.get(reader);
                if (next != '"') {
                    throw std::runtime_error("expected a string as the object key at position " + std::to_string(buffer.position() + 1));
                }
                auto key = extract_string(buffer, reader);
                if (map.find(key) != map.end()) {
                    throw std::runtime_error("detected duplicate keys in the object at position " + std::to_string(buffer.position() + 1));
                }

                chomp(buffer, reader);
                if (!buffer.valid()) {
                    throw std::runtime_error("unterminated object starting at position " + std::to_string(start));
                }
                if (buffer.get(reader) != ':') {
                    throw std::runtime_error("expected ':' to separate keys and values at position " + std::to_string(buffer.position() + 1));
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

                next = buffer.get(reader);
                if (next == '}') {
                    break;
                } else if (next != ',') {
                    throw std::runtime_error("unknown character '" + std::string(1, next) + "' in array at position " + std::to_string(buffer.position() + 1));
                }

                buffer.advance(); 
                chomp(buffer, reader);
                if (!buffer.valid()) {
                    throw std::runtime_error("unterminated object starting at position " + std::to_string(start));
                }
            }
        }

        buffer.advance(); // skip the closing brace.

    } else if (current == '-') {
        buffer.advance(); 
        if (!buffer.valid()) {
            throw std::runtime_error("incomplete number starting at position " + std::to_string(start));
        }
        output.reset(new Number(-extract_number(buffer, reader)));

    } else if (std::isdigit(current)) {
        output.reset(new Number(extract_number(buffer, reader)));

    } else {
        throw std::runtime_error(std::string("unknown type starting with '") + std::string(1, current) + "' at position " + std::to_string(start));
    }

    return output;
}

template<class Reader>
std::shared_ptr<Base> parse_json(Reader& reader) {
    CurrentBuffer buffer(reader);
    chomp(buffer, reader);
    auto output = parse_thing(buffer, reader);
    chomp(buffer, reader);
    if (buffer.valid()) {
        throw std::runtime_error("invalid json with trailing non-space characters at position " + std::to_string(buffer.position() + 1));
    }
    return output;
}

}
/**
 * @endcond
 */



}

#endif
