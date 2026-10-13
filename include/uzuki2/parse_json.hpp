#ifndef UZUKI2_PARSE_JSON_HPP
#define UZUKI2_PARSE_JSON_HPP

#include <memory>
#include <vector>
#include <cctype>
#include <string>
#include <stdexcept>
#include <exception>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <type_traits>
#include <cstddef>

#include "byteme/byteme.hpp"
#include "millijson/millijson.hpp"
#include "ritsuko/ritsuko.hpp"
#include "sanisizer/sanisizer.hpp"

#include "interfaces.hpp"
#include "Dummy.hpp"
#include "ExternalTracker.hpp"
#include "ParsedList.hpp"
#include "utils.hpp"

/**
 * @file parse_json.hpp
 * @brief Parsing methods for JSON files.
 */

namespace uzuki2 {

/**
 * @namespace uzuki2::json
 * @brief Parse an R list from a JSON file.
 *
 * JSON provides an alternative to the HDF5 format handled by `hdf5::parse()` and friends.
 * JSON is simpler to parse and has less formatting-related overhead.
 * However, it does not support random access and discards some precision for floating-point numbers.
 */
namespace json {

/**
 * @cond
 */
inline const std::vector<std::shared_ptr<millijson::Base> >& extract_array(const std::unordered_map<std::string, std::shared_ptr<millijson::Base> >& properties, const std::string& name) {
    auto it = properties.find(name);
    if (it == properties.end()) {
        throw std::runtime_error("no such property");
    }

    const auto& ptr = it->second;
    if (ptr->type() != millijson::ARRAY) {
        throw std::runtime_error("expected an array");
    }

    return static_cast<const millijson::Array*>(ptr.get())->value();
}

inline const millijson::Array* extract_array_or_null(const std::unordered_map<std::string, std::shared_ptr<millijson::Base> >& properties, const std::string& name) {
    auto it = properties.find(name);
    if (it == properties.end()) {
        return NULL;
    }

    const auto ptr = it->second;
    if (ptr->type() != millijson::ARRAY) {
        throw std::runtime_error("expected an array");
    }

    return static_cast<const millijson::Array*>(ptr.get());
}

template<class Destination_>
void fill_names(const millijson::Array* names_ptr, Destination_* dest) {
    const auto& names = names_ptr->value();
    if (names.size() != dest->size()) {
        throw std::runtime_error("unexpected number of names");
    }

    const auto nnames = names.size();
    for (I<decltype(nnames)> i = 0; i < nnames; ++i) {
        if (names[i]->type() != millijson::STRING) {
            throw std::runtime_error("expected a string at element " + std::to_string(i));
        }
        dest->set_name(i, static_cast<const millijson::String*>(names[i].get())->value());
    }
}

template<class Function_>
auto process_array_or_scalar_values(const std::unordered_map<std::string, std::shared_ptr<millijson::Base> >& properties, Function_ fun) {
    const millijson::Array* names_ptr;
    try {
        names_ptr = extract_array_or_null(properties, "names");
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to read 'names'")); 
    }

    auto out_ptr = [&]{
        try {
            auto vIt = properties.find("values");
            if (vIt == properties.end()) {
                throw std::runtime_error("no such property");
            }

            const auto& values_ptr = vIt->second;
            if (values_ptr->type() == millijson::ARRAY) {
                return fun(static_cast<const millijson::Array*>(values_ptr.get())->value(), names_ptr != NULL, false);
            } else {
                std::vector<std::shared_ptr<millijson::Base> > temp { values_ptr };
                return fun(temp, names_ptr != NULL, true);
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to read 'values'"));
        }
    }();

    if (names_ptr != NULL) {
        try {
            fill_names(names_ptr, out_ptr);
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to parse 'names'"));
        }
    }
    return out_ptr;
}

template<class Destination_, class Function_>
void extract_integers(const std::vector<std::shared_ptr<millijson::Base> >& values, Destination_* dest, Function_ check, const Version& version) {
    const auto n = values.size();
    for (I<decltype(n)> i = 0; i < n; ++i) {
        try {
            if (values[i]->type() == millijson::NOTHING) {
                dest->set_missing(i);
                continue;
            }

            if (values[i]->type() != millijson::NUMBER) {
                throw std::runtime_error("expected a number");
            }

            auto val = static_cast<const millijson::Number*>(values[i].get())->value();
            if (val != std::floor(val)) {
                throw std::runtime_error("expected an integer");
            }

            // 32-bit integers are always representable by doubles, as the latter have 53 bits of precision.
            constexpr double upper = std::numeric_limits<std::int32_t>::max();
            constexpr double lower = std::numeric_limits<std::int32_t>::min();
            if (val < lower || val > upper) {
                throw std::runtime_error("value cannot be represented by a 32-bit signed integer");
            }

            const std::int32_t ival = val;
            if (version.equals(1, 0) && ival == -2147483648) {
                dest->set_missing(i);
                continue;
            }

            check(ival);
            dest->set(i, ival);
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to process element " + std::to_string(i)));
        }
    }
}

template<class Destination_, class Function_>
void extract_strings(const std::vector<std::shared_ptr<millijson::Base> >& values, Destination_* dest, Function_ check) {
    const auto n = values.size();
    for (I<decltype(n)> i = 0; i < n; ++i) {
        try {
            if (values[i]->type() == millijson::NOTHING) {
                dest->set_missing(i);
                continue;
            }

            if (values[i]->type() != millijson::STRING) {
                throw std::runtime_error("expected a string");
            }

            const auto& str = static_cast<const millijson::String*>(values[i].get())->value();
            check(str);
            dest->set(i, str);
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to process element " + std::to_string(i)));
        }
    }
}

template<class Provisioner_, class Externals_>
std::shared_ptr<Base> parse_object(const millijson::Base* contents, Externals_& ext, const Version& version) {
    if (contents->type() != millijson::OBJECT) {
        throw std::runtime_error("each R object should be represented by a JSON object");
    }
    const auto& map = static_cast<const millijson::Object*>(contents)->value();

    const auto& type = [&]() -> const std::string& {
        try {
            auto tIt = map.find("type");
            if (tIt == map.end()) {
                throw std::runtime_error("no such property");
            }

            const auto& type_ptr = tIt->second;
            if (type_ptr->type() != millijson::STRING) {
                throw std::runtime_error("expected a string");
            }
            return static_cast<const millijson::String*>(type_ptr.get())->value();
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to read 'type'"));
        }
    }();

    std::shared_ptr<Base> output;
    if (type == "nothing") {
        output.reset(Provisioner_::new_Nothing());

    } else if (type == "external") {
        try {
            auto iIt = map.find("index");
            if (iIt == map.end()) {
                throw std::runtime_error("no such property");
            }
            const auto& index_ptr = iIt->second;
            if (index_ptr->type() != millijson::NUMBER) {
                throw std::runtime_error("expected a number");
            }

            const auto flt_index = static_cast<const millijson::Number*>(index_ptr.get())->value();
            if (flt_index != std::floor(flt_index)) {
                throw std::runtime_error("expected an integer");
            } else if (flt_index < 0) {
                throw std::runtime_error("expected a non-negative integer");
            }

            std::int32_t index;
            try {
                index = sanisizer::from_float<std::int32_t>(flt_index);
            } catch (...) {
                throw std::runtime_error("value should fit in a 32-bit signed integer");
            }
            if (sanisizer::is_greater_than_or_equal(index, ext.size())) {
                throw std::runtime_error("external index out of range");
            }
            output.reset(Provisioner_::new_External(ext.get(index)));
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to read 'index'"));
        }

    } else if (type == "integer") {
        process_array_or_scalar_values(
            map,
            [&](const auto& vals, bool named, bool scalar) -> auto {
                auto ptr = Provisioner_::new_Integer(sanisizer::cast<std::size_t>(vals.size()), named, scalar);
                output.reset(ptr);
                extract_integers(vals, ptr, [](std::int32_t) -> void {}, version);
                return ptr;
            }
        );

    } else if (type == "factor" || (version.equals(1, 0) && type == "ordered")) {
        bool ordered = false;
        if (type == "ordered") {
            ordered = true;
        } else {
            auto oIt = map.find("ordered");
            if (oIt != map.end()) {
                if (oIt->second->type() != millijson::BOOLEAN) {
                    throw std::runtime_error("expected a boolean at 'ordered'");
                }
                ordered = static_cast<const millijson::Boolean*>((oIt->second).get())->value();
            }
        }

        bool level_failed = true;
        try {
            const std::string level_field = "levels"; // use a separate variable to avoid issues with overzealous GCC warnings.
            const auto& lvals = extract_array(map, level_field);
            const auto nlevels = lvals.size();

            level_failed = false;
            auto fptr = process_array_or_scalar_values(
                map,
                [&](const auto& vals, bool named, bool scalar) -> auto {
                    auto ptr = Provisioner_::new_Factor(sanisizer::cast<std::size_t>(vals.size()), named, scalar, sanisizer::cast<std::size_t>(nlevels), ordered);
                    output.reset(ptr);
                    extract_integers(
                        vals,
                        ptr,
                        [&](std::int32_t x) -> void {
                            if (x < 0) {
                                throw std::runtime_error("factor code should be non-negative");
                            } else if (sanisizer::is_greater_than_or_equal(x, nlevels)) {
                                throw std::runtime_error("factor code should be less than the number of levels");
                            }
                        },
                        version
                    );
                    return ptr;
                }
            );

            level_failed = true;
            std::unordered_set<std::string> existing;
            for (I<decltype(nlevels)> l = 0; l < nlevels; ++l) {
                try {
                    if (lvals[l]->type() != millijson::STRING) {
                        throw std::runtime_error("expected a string");
                    }

                    const auto& level = static_cast<const millijson::String*>(lvals[l].get())->value();
                    if (existing.find(level) != existing.end()) {
                        throw std::runtime_error("detected duplicate level '" + level + "'");
                    }
                    fptr->set_level(l, level);
                    existing.insert(level);
                } catch (...) {
                    std::throw_with_nested(std::runtime_error("failed to read element " + std::to_string(l)));
                }
            }
        } catch (...) {
            if (level_failed) {
                std::throw_with_nested(std::runtime_error("failed to read 'levels'")); 
            } else {
                throw;
            }
        }

    } else if (type == "boolean") {
        process_array_or_scalar_values(
            map,
            [&](const auto& vals, bool named, bool scalar) -> auto {
                const auto n = vals.size();
                auto ptr = Provisioner_::new_Boolean(sanisizer::cast<std::size_t>(n), named, scalar);
                output.reset(ptr);

                for (I<decltype(n)> i = 0; i < n; ++i) {
                    try {
                        if (vals[i]->type() == millijson::NOTHING) {
                            ptr->set_missing(i);
                            continue;
                        }

                        if (vals[i]->type() != millijson::BOOLEAN) {
                            throw std::runtime_error("expected a boolean");
                        }
                        ptr->set(i, static_cast<const millijson::Boolean*>(vals[i].get())->value());
                    } catch (...) {
                        std::throw_with_nested(std::runtime_error("failed to read element " + std::to_string(i)));
                    }
                }

                return ptr;
            }
        );

    } else if (type == "number") {
        process_array_or_scalar_values(
            map,
            [&](const auto& vals, bool named, bool scalar) -> auto {
                const auto n = vals.size();
                auto ptr = Provisioner_::new_Number(sanisizer::cast<std::size_t>(n), named, scalar);
                output.reset(ptr);

                for (I<decltype(n)> i = 0; i < n; ++i) {
                    try {
                        if (vals[i]->type() == millijson::NOTHING) {
                            ptr->set_missing(i);
                            continue;
                        }

                        if (vals[i]->type() == millijson::NUMBER) {
                            ptr->set(i, static_cast<const millijson::Number*>(vals[i].get())->value());
                        } else if (vals[i]->type() == millijson::STRING) {
                            auto str = static_cast<const millijson::String*>(vals[i].get())->value();
                            if (str == "NaN") {
                                ptr->set(i, std::numeric_limits<double>::quiet_NaN());
                            } else if (str == "Inf") {
                                ptr->set(i, std::numeric_limits<double>::infinity());
                            } else if (str == "-Inf") {
                                ptr->set(i, -std::numeric_limits<double>::infinity());
                            } else {
                                throw std::runtime_error("unsupported string '" + str + "'");
                            }
                        } else {
                            throw std::runtime_error("expected a number");
                        }
                    } catch (...) {
                        std::throw_with_nested(std::runtime_error("failed to read element " + std::to_string(i)));
                    }
                }

                return ptr;
            }
        );

    } else if (type == "string" || (version.equals(1, 0) && (type == "date" || type == "date-time"))) {
        StringVector::Format format = StringVector::NONE;
        if (version.equals(1, 0)) {
            if (type == "date") {
                format = StringVector::DATE;
            } else if (type == "date-time") {
                format = StringVector::DATETIME;
            }
        } else {
            auto fIt = map.find("format");
            if (fIt != map.end()) {
                if (fIt->second->type() != millijson::STRING) {
                    throw std::runtime_error("expected a string at 'format'");
                }
                auto fptr = static_cast<const millijson::String*>(fIt->second.get());
                if (fptr->value() == "date") {
                    format = StringVector::DATE;
                } else if (fptr->value() == "date-time") {
                    format = StringVector::DATETIME;
                } else {
                    throw std::runtime_error("unsupported format '" + fptr->value() + "' at 'format'");
                }
            }
        }

        process_array_or_scalar_values(
            map,
            [&](const auto& vals, bool named, bool scalar) -> auto {
                auto ptr = Provisioner_::new_String(sanisizer::cast<std::size_t>(vals.size()), named, scalar, format);
                output.reset(ptr);

                if (format == StringVector::NONE) {
                    extract_strings(vals, ptr, [](const std::string&) -> void {});
                } else if (format == StringVector::DATE) {
                    extract_strings(
                        vals,
                        ptr,
                        [&](const std::string& x) -> void {
                            if (!ritsuko::is_date(x.c_str(), x.size())) {
                                 throw std::runtime_error("dates should follow YYYY-MM-DD formatting");
                            }
                        }
                    );
                } else if (format == StringVector::DATETIME) {
                    extract_strings(
                        vals,
                        ptr,
                        [&](const std::string& x) -> void {
                            if (!ritsuko::is_rfc3339(x.c_str(), x.size())) {
                                 throw std::runtime_error("date-times should follow the Internet Date/Time format");
                            }
                        }
                    );
                }

                return ptr;
            }
        );

    } else if (type == "list") {
        const millijson::Array* names_ptr;
        try {
            names_ptr = extract_array_or_null(map, "names");
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to read 'names'"));
        }

        List* lptr;
        try {
            const std::string value_field = "values"; // use a separate variable to avoid issues with overzealous GCC warnings.
            const auto& vals = extract_array(map, value_field);

            const auto n = vals.size();
            lptr = Provisioner_::new_List(sanisizer::cast<std::size_t>(n), names_ptr != NULL);
            output.reset(lptr);

            for (I<decltype(n)> i = 0; i < n; ++i) {
                try {
                    lptr->set(i, parse_object<Provisioner_>(vals[i].get(), ext, version));
                } catch (...) {
                    std::throw_with_nested(std::runtime_error("failed to read element " + std::to_string(i))); 
                }
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to read 'values'")); 
        }

        if (names_ptr != NULL) {
            try {
                fill_names(names_ptr, lptr);
            } catch (...) {
                std::throw_with_nested(std::runtime_error("failed to read 'names'"));
            }
        }

    } else {
        throw std::runtime_error("unknown object type '" + type + "' in 'type'");
    }

    return output;
}
/**
 * @endcond
 */

/**
 * @brief Options for JSON file parsing.
 */
struct Options {
    /**
     * Whether parsing should be done in parallel to file I/O.
     * If true, an extra thread is used to avoid blocking I/O operations.
     */
    bool parallel = false;

    /**
     * Whether to throw an error if the top-level R object is not an R list.
     */
    bool strict_list = true;

    /**
     * Size of the buffer to use for reading and decompressing bytes.
     * Larger values may improve speed at the cost of memory usage.
     */
    std::size_t buffer_size = sanisizer::cap<std::size_t>(65536);
};

/**
 * Parse JSON file contents using the **uzuki2** specification, given an arbitrary input source of bytes.
 *
 * @tparam Provisioner_ A class namespace defining static methods for creating new `Base` objects.
 * See `hdf5::parse()` for more details. 
 * @tparam Reader_ Class of the source of input bytes.
 * This should satisfy the `byteme::Reader` interface.
 * @tparam Externals_ Class describing how to resolve external references for type `EXTERNAL`.
 * See `hdf5::parse()` for more details. 
 *
 * @param reader Source of input bytes representing the contents of the JSON file.
 * @param ext Instance of an external reference resolver class.
 * @param options Options for parsing.
 *
 * @return A `ParsedList` containing a pointer to the root `Base` object.
 * Depending on `Provisioner_`, this may contain references to all nested objects.
 *
 * @throw std::exception Thrown upon encountering an invalid representation in `reader`. 
 * This may be a `std::nested_exception` in which case the nested exceptions should be extracted for an informative error message.
 */
template<class Provisioner_, class Reader_, class Externals_>
ParsedList parse(Reader_& reader, Externals_ ext, const Options& options) {
    millijson::ParseOptions popt;
    popt.buffer_size = options.buffer_size;
    popt.parallel = options.parallel;
    auto contents = millijson::parse(reader, popt);

    Version version;
    if (contents->type() == millijson::OBJECT) {
        const auto& map = static_cast<const millijson::Object*>(contents.get())->value();
        auto vIt = map.find("version");
        if (vIt != map.end()) {
            try {
                if (vIt->second->type() != millijson::STRING) {
                    throw std::runtime_error("expected a string");
                }
                const auto& vstr = static_cast<const millijson::String*>(vIt->second.get())->value();
                auto vraw = ritsuko::parse_version_string(vstr.c_str(), vstr.size(), /* skip_patch = */ true);
                version.major = vraw.major;
                version.minor = vraw.minor;
            } catch (...) {
                std::throw_with_nested(std::runtime_error("failed to read 'version'"));
            }
        }
    }

    ExternalTracker etrack(std::move(ext));
    auto output = parse_object<Provisioner_>(contents.get(), etrack, version);

    if (options.strict_list && output->type() != LIST) {
        throw std::runtime_error("top-level object should represent an R list");
    }
    etrack.validate();

    return ParsedList(std::move(output), std::move(version));
}

/**
 * Parse JSON file contents using the **uzuki2** specification, given the file path.
 *
 * @tparam Provisioner_ A class namespace defining static methods for creating new `Base` objects.
 * See `hdf5::parse()` for more details. 
 * @tparam Externals_ Class describing how to resolve external references for type `EXTERNAL`.
 * See `hdf5::parse()` for more details. 
 *
 * @param file Path to a (possibly Gzip-compressed) JSON file.
 * @param ext Instance of an external reference resolver class.
 * @param options Options for parsing.
 *
 * @return A `ParsedList` containing a pointer to the root `Base` object.
 * Depending on `Provisioner_`, this may contain references to all nested objects.
 *
 * @throw std::exception Thrown upon encountering an invalid representation in `reader`. 
 * This may be a `std::nested_exception` in which case the nested exceptions should be extracted for an informative error message.
 */
template<class Provisioner_, class Externals_>
ParsedList parse_file(const std::string& file, Externals_ ext, const Options& options) {
    std::unique_ptr<byteme::Reader> ptr;
    if (byteme::is_gzip(file.c_str())) {
        ptr.reset(new byteme::GzipFileReader(file.c_str(), {}));
    } else {
        ptr.reset(new byteme::RawFileReader(file.c_str(), {}));
    }
    return parse<Provisioner_>(*ptr, std::move(ext), options);
}

/**
 * Parse a buffer containing JSON file contents using the **uzuki2** specification. 
 *
 * @tparam Provisioner_ A class namespace defining static methods for creating new `Base` objects.
 * See `hdf5::parse()` for more details. 
 * @tparam Externals_ Class describing how to resolve external references for type `EXTERNAL`.
 * See `hdf5::parse()` for more details. 
 *
 * @param[in] buffer Pointer to an array containing the JSON file contents (possibly Gzip/Zlib-compressed).
 * @param len Length of the buffer in bytes.
 * @param ext Instance of an external reference resolver class.
 * @param options Options for parsing.
 *
 * @return A `ParsedList` containing a pointer to the root `Base` object.
 * Depending on `Provisioner_`, this may contain references to all nested objects.
 *
 * @throw std::exception Thrown upon encountering an invalid representation in `reader`. 
 * This may be a `std::nested_exception` in which case the nested exceptions should be extracted for an informative error message.
 */
template<class Provisioner_, class Externals_>
ParsedList parse_buffer(const unsigned char* buffer, std::size_t len, Externals_ ext, const Options& options) {
    std::unique_ptr<byteme::Reader> ptr;
    if (byteme::is_zlib_or_gzip(buffer, len)) {
        ptr.reset(new byteme::ZlibBufferReader(buffer, len, {}));
    } else {
        ptr.reset(new byteme::RawBufferReader(buffer, len));
    }
    return parse<Provisioner_>(*ptr, std::move(ext), options);
}

/**
 * Validate JSON file contents against the **uzuki2** specification, given a source of bytes.
 * Any invalid representations will cause an error to be thrown.
 *
 * @param reader Instance of a `byteme::Reader` providing the contents of the JSON file.
 * @param num_external Expected number of external references. 
 * @param options Options for parsing.
 *
 * @throw std::exception Thrown upon encountering an invalid representation. 
 * This may be a `std::nested_exception` in which case the nested exceptions should be extracted for an informative error message.
 */
inline void validate(byteme::Reader& reader, int num_external, const Options& options) {
    parse<DummyProvisioner>(reader, DummyExternals(num_external), options);
}

/**
 * Validate JSON file contents against the **uzuki2** specification, given a path to the file.
 * Any invalid representations will cause an error to be thrown.
 *
 * @param file Path to a (possible Gzip-compressed) JSON file.
 * @param num_external Expected number of external references. 
 * @param options Options for parsing.
 *
 * @throw std::exception Thrown upon encountering an invalid representation in `file`. 
 * This may be a `std::nested_exception` in which case the nested exceptions should be extracted for an informative error message.
 */
inline void validate_file(const std::string& file, int num_external, const Options& options) {
    parse_file<DummyProvisioner>(file, DummyExternals(num_external), options);
}

/**
 * Validate JSON file contents against the **uzuki2** specification, given a buffer containing the file contents.
 * Any invalid representations will cause an error to be thrown.
 *
 * @param[in] buffer Pointer to an array containing the JSON file contents (possibly Gzip/Zlib-compressed).
 * @param len Length of the buffer in bytes.
 * @param num_external Expected number of external references. 
 * @param options Options for parsing.
 *
 * @throw std::exception Thrown upon encountering an invalid representation in `buffer`. 
 * This may be a `std::nested_exception` in which case the nested exceptions should be extracted for an informative error message.
 */
inline void validate_buffer(const unsigned char* buffer, std::size_t len, int num_external, const Options& options) {
    parse_buffer<DummyProvisioner>(buffer, len, DummyExternals(num_external), options);
}

}

}

#endif
