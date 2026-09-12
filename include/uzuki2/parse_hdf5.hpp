#ifndef UZUKI2_PARSE_HPP
#define UZUKI2_PARSE_HPP

#include <memory>
#include <vector>
#include <cctype>
#include <string>
#include <cstring>
#include <stdexcept>
#include <cstdint>
#include <unordered_set>

#include "H5Cpp.h"

#include "interfaces.hpp"
#include "Dummy.hpp"
#include "ExternalTracker.hpp"
#include "Version.hpp"
#include "ParsedList.hpp"

#include "ritsuko/ritsuko.hpp"

/**
 * @file parse_hdf5.hpp
 * @brief Parsing methods for HDF5 files.
 */

namespace uzuki2 {

/**
 * @namespace uzuki2::hdf5
 * @brief Parse an R list from a HDF5 file.
 *
 * The hierarchical nature of HDF5 allows it to naturally store nested list structures.
 * It supports random access of list components, which provides some optimization opportunities for parsing large lists.
 * However, it incurs a large overhead per list element; for small lists, users may prefer to use a JSON file instead (see `json`).
 */
namespace hdf5 {

/**
 * @cond
 */
inline void validate_numeric_missing_placeholder(const H5::Attribute& attr, const H5::DataSet& data, const Version& version) { 
    if (attr.getSpace().getSimpleExtentNdims() != 0) {
        throw std::runtime_error("expected the '" + ritsuko::hdf5::get_name(attr) + "' attribute to be a scalar");
    }
    if (version.lt(1, 2)) {
        if (attr.getDataType().getClass() != data.getDataType().getClass()) {
            throw std::runtime_error("expected the '" + ritsuko::hdf5::get_name(attr) + "' attribute to have the same type class as its dataset");
        }
    } else {
        if (attr.getDataType() != data.getDataType()) {
            throw std::runtime_error("expected the '" + ritsuko::hdf5::get_name(attr) + "' attribute to have the same type as its dataset");
        }
    }
}

inline void validate_string_missing_placeholder(const H5::Attribute& attr) {
    if (attr.getSpace().getSimpleExtentNdims() != 0) {
        throw std::runtime_error("expected the '" + ritsuko::hdf5::get_name(attr) + "' attribute to be a scalar");
    }
    if (!ritsuko::hdf5::is_utf8_string(attr)) {
        throw std::runtime_error("expected the '" + ritsuko::hdf5::get_name(attr) + "' attribute to be a UTF-8 string");
    }
}

template<class Host_, class Function_>
void parse_integer_like(const H5::DataSet& handle, Host_* ptr, bool is_scalar, Function_ check, const Version& version) try {
    if (ritsuko::hdf5::exceeds_integer_limit(handle, 32, true)) {
        throw std::runtime_error("dataset cannot be represented by 32-bit signed integers");
    }

    bool has_missing = false;
    int32_t missing_value = -2147483648;
    if (version.equals(1, 0)) {
        has_missing = true;
    } else {
        const char* placeholder_name = "missing-value-placeholder";
        has_missing = handle.attrExists(placeholder_name);
        if (has_missing) {
            auto attr = handle.openAttribute(placeholder_name);
            validate_numeric_missing_placeholder(attr, handle, version);
            attr.read(H5::PredType::NATIVE_INT32, &missing_value);
        }
    }

    auto set = [&](hsize_t i, int32_t x) -> void {
        if (has_missing && x == missing_value) {
            ptr->set_missing(i);
        } else {
            check(x);
            ptr->set(i, x);
        }
    };

    if (is_scalar) {
        int32_t value;
        handle.read(&value, H5::PredType::NATIVE_INT32);
        set(0, value);
    } else {
        hsize_t full_length = ptr->size();
        ritsuko::hdf5::Stream1dNumericDataset<int32_t> stream(&handle, full_length);
        std::vector<int32_t> buffer(stream.chunk_size());
        while (true) {
            const auto available = stream.load(buffer.data());
            if (available == 0) {
                break;
            }
            for (hsize_t i = 0; i < available; ++i) {
                set(i + stream.start(), buffer[i]);
            }
        }
    }

} catch (std::exception& e) {
    throw std::runtime_error("failed to load integer dataset at '" + ritsuko::hdf5::get_name(handle) + "'; " + std::string(e.what()));
}

template<class Host_, class Function_>
void parse_string_like(const H5::DataSet& handle, Host_* ptr, bool is_scalar, Function_ check) try {
    if (!ritsuko::hdf5::is_utf8_string(handle)) {
        throw std::runtime_error("expected a datatype that can be represented by a UTF-8 string");
    }

    std::optional<std::string> missingness;
    const char* placeholder_name = "missing-value-placeholder";
    if (handle.attrExists(placeholder_name)) {
        auto attr = handle.openAttribute(placeholder_name);
        validate_string_missing_placeholder(attr);
        missingness = ritsuko::hdf5::read_scalar_string(attr);
    }

    auto set = [&](hsize_t i, std::string x) -> void { 
        if (missingness.has_value() && x == *missingness) {
            ptr->set_missing(i);
        } else {
            check(x);
            ptr->set(i, std::move(x));
        }
    };

    if (is_scalar) {
        auto x = ritsuko::hdf5::read_scalar_string(handle);
        set(0, std::move(x));
    } else {
        hsize_t full_length = ptr->size();
        ritsuko::hdf5::Stream1dStringDataset stream(&handle, full_length);
        std::vector<std::string> buffer(stream.chunk_size());
        while (true) {
            const auto available = stream.load(buffer.data());
            if (available == 0) {
                break;
            }
            for (hsize_t i = 0; i < available; ++i) {
                set(i + stream.start(), std::move(buffer[i]));
            }
        }
    }

} catch (std::exception& e) {
    throw std::runtime_error("failed to load string dataset at '" + ritsuko::hdf5::get_name(handle) + "'; " + std::string(e.what()));
}

inline double r_missing_value() {
    uint32_t tmp_value = 1;
    auto tmp_ptr = reinterpret_cast<unsigned char*>(&tmp_value);

    // Mimic R's generation of these values, but we can't use type punning as
    // this is not legal in C++, and we don't have bit_cast yet.
    double missing_value = 0;
    auto missing_ptr = reinterpret_cast<unsigned char*>(&missing_value);

    int step = 1;
    if (tmp_ptr[0] == 1) { // little-endian. 
        missing_ptr += sizeof(double) - 1;
        step = -1;
    }

    *missing_ptr = 0x7f;
    *(missing_ptr += step) = 0xf0;
    *(missing_ptr += step) = 0x00;
    *(missing_ptr += step) = 0x00;
    *(missing_ptr += step) = 0x00;
    *(missing_ptr += step) = 0x00;
    *(missing_ptr += step) = 0x07;
    *(missing_ptr += step) = 0xa2;

    return missing_value;
}

template<class Host_, class Function_>
void parse_numbers(const H5::DataSet& handle, Host_* ptr, bool is_scalar, Function_ check, const Version& version) try {
    if (version.lt(1, 3)) {
        if (handle.getTypeClass() != H5T_FLOAT) {
            throw std::runtime_error("expected a floating-point dataset");
        }
    } else {
        if (ritsuko::hdf5::exceeds_float_limit(handle, 64)) {
            throw std::runtime_error("dataset cannot be represented by 64-bit floats");
        }
    }

    // Check that we support IEEE754-compliant floats.
    static_assert(std::numeric_limits<double>::is_iec559);

    bool has_missing = false;
    double missing_value = 0;
    if (version.equals(1, 0)) {
        has_missing = true;
        missing_value = r_missing_value();
    } else {
        const char* placeholder_name = "missing-value-placeholder";
        has_missing = handle.attrExists(placeholder_name);
        if (has_missing) {
            auto attr = handle.openAttribute(placeholder_name);
            validate_numeric_missing_placeholder(attr, handle, version);
            attr.read(H5::PredType::NATIVE_DOUBLE, &missing_value);
        }
    }

    bool should_compare_nan = version.lt(1, 3);
    bool is_placeholder_nan = std::isnan(missing_value);
    auto is_missing_value = [&](double val) -> bool {
        if (should_compare_nan) {
            auto xptr = reinterpret_cast<const unsigned char*>(&missing_value);
            auto yptr = reinterpret_cast<const unsigned char*>(&val);
            return std::memcmp(xptr, yptr, sizeof(double)) == 0;
        } else if (is_placeholder_nan) {
            return std::isnan(val);
        } else {
            return val == missing_value;
        }
    };

    auto set = [&](hsize_t i, double x) -> void {
        if (has_missing && is_missing_value(x)) {
            ptr->set_missing(i);
        } else {
            check(x);
            ptr->set(i, x);
        }
    };

    if (is_scalar) {
        double val;
        handle.read(&val, H5::PredType::NATIVE_DOUBLE);
        set(0, val);
    } else {
        hsize_t full_length = ptr->size();
        ritsuko::hdf5::Stream1dNumericDataset<double> stream(&handle, full_length);
        std::vector<double> buffer(stream.chunk_size());
        while (true) {
            const auto available = stream.load(buffer.data());
            if (available == 0) {
                break;
            }
            for (hsize_t i = 0; i < available; ++i) {
                set(i + stream.start(), buffer[i]);
            }
        }
    }

} catch (std::exception& e) {
    throw std::runtime_error("failed to load floating-point dataset at '" + ritsuko::hdf5::get_name(handle) + "'; " + std::string(e.what()));
}

template<class Host_>
void extract_names(const H5::Group& handle, Host_* ptr) try {
    auto nhandle = handle.openDataSet("names");
    if (!ritsuko::hdf5::is_utf8_string(nhandle)) {
        throw std::runtime_error("expected 'names' to use a datatype that can be represented by a UTF-8 string");
    }

    size_t len = ptr->size();
    const auto space = nhandle.getSpace();
    if (space.getSimpleExtentNdims() != 1) {
        throw std::runtime_error("expected 'names' to be a 1-dimensional dataset");
    }
    hsize_t nlen;
    space.getSimpleExtentDims(&nlen);
    if (nlen != len) {
        throw std::runtime_error("number of names should be equal to the object length");
    }

    ritsuko::hdf5::Stream1dStringDataset stream(&nhandle, nlen);
    std::vector<std::string> buffer(stream.chunk_size());
    while (true) {
        const auto available = stream.load(buffer.data());
        if (available == 0) {
            break;
        }
        for (hsize_t i = 0; i < available; ++i) {
            ptr->set_name(i + stream.start(), std::move(buffer[i]));
        }
    }

} catch (std::exception& e) {
    throw std::runtime_error("failed to load names at '" + ritsuko::hdf5::get_name(handle) + "'; " + std::string(e.what()));
}

inline std::string read_uzuki_attr(const H5::Group& handle, const char* name) {
    const auto attr = handle.openAttribute(name); 
    if (attr.getSpace().getSimpleExtentNdims() != 0) {
        throw std::runtime_error("'" + std::string(name) + "' should be a scalar attribute in '" + ritsuko::hdf5::get_name(handle) + "'");
    }
    if (!ritsuko::hdf5::is_utf8_string(attr)) {
        throw std::runtime_error("'" + std::string(name) + "' should be stored as a UTF-8 string in '" + ritsuko::hdf5::get_name(handle) + "'");
    }
    return ritsuko::hdf5::read_scalar_string(attr);
}

template<class Provisioner_, class Externals_>
std::shared_ptr<Base> parse_inner(const H5::Group& handle, Externals_& ext, const Version& version) try {
    auto object_type = read_uzuki_attr(handle, "uzuki_object");
    std::shared_ptr<Base> output;

    if (object_type == "list") {
        auto dhandle = handle.openGroup("data");
        size_t len = dhandle.getNumObjs();

        bool named = handle.exists("names");
        auto lptr = Provisioner_::new_List(len, named);
        output.reset(lptr);

        try {
            for (size_t i = 0; i < len; ++i) {
                auto istr = std::to_string(i);
                auto lhandle = dhandle.openGroup(istr);
                lptr->set(i, parse_inner<Provisioner_>(lhandle, ext, version));
            }
        } catch (std::exception& e) {
            throw std::runtime_error("failed to parse list contents in 'data'; " + std::string(e.what()));
        }

        if (named) {
            extract_names(handle, lptr);
        }

    } else if (object_type == "vector") {
        auto dhandle = handle.openDataSet("data");
        const auto dspace = dhandle.getSpace();
        const auto ndims = dspace.getSimpleExtentNdims();

        hsize_t len = 1;
        bool is_scalar = false;
        if (ndims == 0) {
            is_scalar = true;
        } else if (ndims == 1) {
            dspace.getSimpleExtentDims(&len);
        } else {
            throw std::runtime_error("expected a 1-dimensional dataset in 'data'");
        }

        const bool named = handle.exists("names");
        auto vector_type = read_uzuki_attr(handle, "uzuki_type");
        if (vector_type == "integer") {
            auto iptr = Provisioner_::new_Integer(len, named, is_scalar);
            output.reset(iptr);
            parse_integer_like(
                dhandle,
                iptr,
                is_scalar,
                [](int32_t) -> void {},
                version
            );

        } else if (vector_type == "boolean") {
            auto bptr = Provisioner_::new_Boolean(len, named, is_scalar);
            output.reset(bptr);
            parse_integer_like(
                dhandle,
                bptr,
                is_scalar,
                [&](int32_t x) -> void { 
                    if (x != 0 && x != 1) {
                        throw std::runtime_error("boolean values should be 0 or 1");
                    }
                },
                version
            );

        } else if (vector_type == "factor" || (version.equals(1, 0) && vector_type == "ordered")) {
            auto levhandle = handle.openDataSet("levels");
            if (!ritsuko::hdf5::is_utf8_string(levhandle)) {
                throw std::runtime_error("expected a datatype that can be represented by a UTF-8 string for 'levels'");
            }

            hsize_t raw_levlen;
            auto lspace = levhandle.getSpace();
            if (lspace.getSimpleExtentNdims() != 1) {
                throw std::runtime_error("expected a 1-dimensional dataset for 'levels'");
            }
            lspace.getSimpleExtentDims(&raw_levlen);
            int32_t levlen = raw_levlen; // needs sanisizer.

            bool ordered = false;
            if (vector_type == "ordered") {
                ordered = true;
            } else if (handle.exists("ordered")) {
                auto ohandle = handle.openDataSet("ordered");
                if (ohandle.getSpace().getSimpleExtentNdims() != 0) {
                    throw std::runtime_error("expected 'ordered' to be a scalar dataset");
                }
                if (ritsuko::hdf5::exceeds_integer_limit(ohandle, 32, true)) {
                    throw std::runtime_error("'ordered' value cannot be represented by a 32-bit integer");
                }
                int32_t tmp_ordered = 0;
                ohandle.read(&tmp_ordered, H5::PredType::NATIVE_INT32);
                ordered = tmp_ordered > 0;
            }

            auto fptr = Provisioner_::new_Factor(len, named, is_scalar, levlen, ordered);
            output.reset(fptr);
            parse_integer_like(
                dhandle,
                fptr,
                is_scalar,
                [&](int32_t x) -> void { 
                    if (x < 0 || x >= levlen) {
                        throw std::runtime_error("factor codes should be non-negative and less than the number of levels");
                    }
                },
                version
            );

            std::unordered_set<std::string> present;
            ritsuko::hdf5::Stream1dStringDataset stream(&levhandle, levlen);
            std::vector<std::string> buffer(stream.chunk_size());
            while (true) {
                const auto available = stream.load(buffer.data());
                if (available == 0) {
                    break;
                }
                for (hsize_t i = 0; i < available; ++i) {
                    auto& x = buffer[i];
                    if (present.find(x) != present.end()) {
                        throw std::runtime_error("levels should be unique");
                    }
                    fptr->set_level(i + stream.start(), x); 
                    present.insert(std::move(x));
                }
            }

        } else if (vector_type == "vls" && !version.lt(1, 4)) {
            constexpr auto precision = std::numeric_limits<uint64_t>::digits;
            ritsuko::cvls::validate_pointer_datatype(dhandle, precision, precision);
            auto hhandle = handle.openDataSet("heap");
            ritsuko::cvls::validate_heap(hhandle);

            const char* placeholder_name = "missing-value-placeholder";
            std::optional<std::string> missingness;
            if (dhandle.attrExists(placeholder_name)) {
                auto attr = dhandle.openAttribute(placeholder_name);
                validate_string_missing_placeholder(attr);
                missingness = ritsuko::hdf5::read_scalar_string(attr);
            }

            auto ptr = Provisioner_::new_String(len, named, is_scalar, StringVector::NONE);
            output.reset(ptr);

            auto set = [&](hsize_t i, std::string x) -> void { 
                if (missingness.has_value() && x == *missingness) {
                    ptr->set_missing(i);
                } else {
                    ptr->set(i, std::move(x));
                }
            };

            if (is_scalar) {
                ritsuko::cvls::Pointer<uint64_t, uint64_t> vlsptr;
                dhandle.read(&vlsptr, ritsuko::cvls::define_pointer_datatype<uint64_t, uint64_t>());

                hsize_t hlen;
                hhandle.getSpace().getSimpleExtentDims(&hlen);
                if (ritsuko::cvls::is_Pointer_out_of_range(vlsptr, hlen)) {
                    throw std::runtime_error("compressed VLS pointer in '" + ritsuko::hdf5::get_name(dhandle) + "' is out of range of the heap");
                }

                H5::DataSpace dspace(1, &hlen);
                const hsize_t len = vlsptr.length; // cast is safe if pointer is within range.
                const hsize_t offset = vlsptr.offset;
                dspace.selectHyperslab(H5S_SELECT_SET, &len, &offset);
                H5::DataSpace mspace(1, &len);

                std::vector<uint8_t> buffer(vlsptr.length);
                hhandle.read(buffer.data(), H5::PredType::NATIVE_UINT8, mspace, dspace);
                auto cptr = reinterpret_cast<const char*>(buffer.data());
                set(0, std::string(cptr, cptr + ritsuko::hdf5::strnlen(cptr, vlsptr.length)));

            } else {
                ritsuko::cvls::Stream1dArray<uint64_t, uint64_t> stream(&dhandle, len, &hhandle);
                std::vector<std::string> buffer(stream.chunk_size());
                while (1) {
                    const auto available = stream.load(buffer.data());
                    if (available == 0) {
                        break;
                    }
                    for (hsize_t i = 0; i < available; ++i) {
                        set(i + stream.start(), std::move(buffer[i]));
                    }
                }
            }

        } else if (vector_type == "string" || (version.equals(1, 0) && (vector_type == "date" || vector_type == "date-time"))) {
            StringVector::Format format = StringVector::NONE;
            if (version.equals(1, 0)) {
                if (vector_type == "date") {
                    format = StringVector::DATE;
                } else if (vector_type == "date-time") {
                    format = StringVector::DATETIME;
                }

            } else if (handle.exists("format")) {
                auto fhandle = handle.openDataSet("format");
                if (fhandle.getSpace().getSimpleExtentNdims() != 0) {
                    throw std::runtime_error("expected 'format' to be a scalar dataset");
                }
                if (!ritsuko::hdf5::is_utf8_string(fhandle)) {
                    throw std::runtime_error("expected 'format' to use a datatype that can be represented by a UTF-8 encoded string");
                }
                auto x = ritsuko::hdf5::read_scalar_string(fhandle);
                if (x == "date") {
                    format = StringVector::DATE;
                } else if (x == "date-time") {
                    format = StringVector::DATETIME;
                } else {
                    throw std::runtime_error("unsupported format '" + x + "'");
                }
            }

            auto sptr = Provisioner_::new_String(len, named, is_scalar, format);
            output.reset(sptr);
            if (format == StringVector::NONE) {
                parse_string_like(
                    dhandle,
                    sptr,
                    is_scalar,
                    [](const std::string&) -> void {}
                );

            } else if (format == StringVector::DATE) {
                parse_string_like(
                    dhandle,
                    sptr,
                    is_scalar,
                    [&](const std::string& x) -> void {
                        if (!ritsuko::is_date(x.c_str(), x.size())) {
                             throw std::runtime_error("dates should follow YYYY-MM-DD formatting");
                        }
                    }
                );

            } else if (format == StringVector::DATETIME) {
                parse_string_like(
                    dhandle,
                    sptr,
                    is_scalar,
                    [&](const std::string& x) -> void {
                        if (!ritsuko::is_rfc3339(x.c_str(), x.size())) {
                             throw std::runtime_error("date-times should follow the Internet Date/Time format");
                        }
                    }
                );
            }

        } else if (vector_type == "number") {
            auto dptr = Provisioner_::new_Number(len, named, is_scalar);
            output.reset(dptr);
            parse_numbers(
                dhandle,
                dptr,
                is_scalar,
                [](double) -> void {},
                version
            );

        } else {
            throw std::runtime_error("unknown vector type '" + vector_type + "'");
        }

        if (named) {
            auto vptr = static_cast<Vector*>(output.get());
            extract_names(handle, vptr);
        }

    } else if (object_type == "nothing") {
        output.reset(Provisioner_::new_Nothing());

    } else if (object_type == "external") {
        auto ihandle = handle.openDataSet("index");
        if (ritsuko::hdf5::exceeds_integer_limit(ihandle, 32, true)) {
            throw std::runtime_error("external index at 'index' cannot be represented by a 32-bit signed integer");
        }

        if (ihandle.getSpace().getSimpleExtentNdims() != 0) {
            throw std::runtime_error("expected scalar dataset at 'index'");
        } 

        int32_t idx;
        ihandle.read(&idx, H5::PredType::NATIVE_INT32);
        if (idx < 0 || static_cast<size_t>(idx) >= ext.size()) {
            throw std::runtime_error("external index out of range at 'index'");
        }

        output.reset(Provisioner_::new_External(ext.get(idx)));

    } else {
        throw std::runtime_error("unknown uzuki2 object type '" + object_type + "'");
    }

    return output;
} catch (std::exception& e) {
    throw std::runtime_error("failed to load object at '" + ritsuko::hdf5::get_name(handle) + "'; " + std::string(e.what()));
    return nullptr; // for consistency.
}
/**
 * @endcond
 */

/**
 * @brief Options for HDF5 file parsing.
 */
struct Options {
    /**
     * Buffer size, in terms of the number of elements, to use for reading data from HDF5 datasets.
     */
    hsize_t buffer_size = 10000;

    /**
     * Whether to throw an error if the top-level R object is not an R list.
     */
    bool strict_list = true;
};

/**
 * @tparam Provisioner_ A class namespace defining static methods for creating new `Base` objects.
 * @tparam Externals_ Class describing how to resolve external references for type `EXTERNAL`.
 *
 * @param handle Handle for a HDF5 group corresponding to the list.
 * @param ext Instance of an external reference resolver class.
 * @param options Optional parameters.
 *
 * @return A `ParsedList` containing a pointer to the root `Base` object.
 * Depending on `Provisioner_`, this may contain references to all nested objects. 
 * 
 * Any invalid representations in `contents` will cause an error to be thrown.
 *
 * @section provisioner-contract Provisioner requirements
 * The `Provisioner_` class is expected to provide the following static methods:
 *
 * - `Nothing* new_Nothing()`, which returns a new instance of a `Nothing` subclass.
 * - `Other* new_Other(void* p)`, which returns a new instance of a `Other` subclass.
 *   `p` is a pointer to an "external" object, generated by calling `ext.get()` (see below).
 * - `List* new_List(size_t l, bool n)`, which returns a new instance of a `List` with length `l`.
 *   If `n = true`, names are present and will be added via `List::set_name()`.
 * - `IntegerVector* new_Integer(size_t l, bool n, bool s)`, which returns a new instance of an `IntegerVector` subclass of length `l`.
 *   If `n = true`, names are present and will be added via `Vector::set_name()`.
 *   If `s = true` and `l = 1`, the value was represented on file as a scalar integer.
 * - `NumberVector* new_Number(size_t l, bool n, bool s)`, which returns a new instance of a `NumberVector` subclass of length `l`.
 *   If `n = true`, names are present and will be added via `Vector::set_name()`.
 *   If `s = true` and `l = 1`, the value was represented on file as a scalar float.
 * - `StringVector* new_String(size_t l, bool n, bool s, StringVector::Format f)`, which returns a new instance of a `StringVector` subclass of length `l` with format `f`.
 *   If `n = true`, names are present and will be added via `Vector::set_name()`.
 *   If `s = true` and `l = 1`, the value was represented on file as a scalar string.
 * - `BooleanVector* new_Boolean(size_t l, bool n, bool s)`, which returns a new instance of a `BooleanVector` subclass of length `l`.
 *   If `n = true`, names are present and will be added via `Vector::set_name()`.
 *   If `s = true` and `l = 1`, the value was represented on file as a scalar boolean.
 * - `Factor* new_Factor(size_t l, bool n, bool s, size_t ll, bool o)`, which returns a new instance of a `Factor` subclass of length `l` and with `ll` unique levels.
 *   If `n = true`, names are present and will be added via `Vector::set_name()`.
 *   If `s = true` and `l = 1`, the lone index was represented on file as a scalar integer.
 *   If `o = true`, the levels should be assumed to be sorted.
 *
 * @section external-contract Externals requirements
 * The `Externals_` class is expected to provide the following `const` methods:
 *
 * - `void* get(size_t i) const`, which returns a pointer to an "external" object, given the index of that object.
 *   This will be stored in the corresponding `Other` subclass generated by `Provisioner_::new_External`.
 * - `size_t size()`, which returns the number of available external references.
 */
template<class Provisioner_, class Externals_>
ParsedList parse(const H5::Group& handle, Externals_ ext, const Options& options) {
    Version version;
    if (handle.attrExists("uzuki_version")) {
        auto ver_str = read_uzuki_attr(handle, "uzuki_version");
        auto vraw = ritsuko::parse_version_string(ver_str.c_str(), ver_str.size(), /* skip_patch = */ true);
        version.major = vraw.major;
        version.minor = vraw.minor;
    }

    ExternalTracker etrack(std::move(ext));
    auto ptr = parse_inner<Provisioner_>(handle, etrack, version);

    if (options.strict_list && ptr->type() != LIST) {
        throw std::runtime_error("top-level object should represent an R list");
    }
    etrack.validate();

    return ParsedList(std::move(ptr), std::move(version));
}

/**
 * Parse HDF5 file contents using the **uzuki2** specification, given the file path.
 *
 * @tparam Provisioner_ A class namespace defining static methods for creating new `Base` objects.
 * @tparam Externals_ Class describing how to resolve external references for type `EXTERNAL`.
 *
 * @param file Path to a HDF5 file.
 * @param name Name of the HDF5 group containing the list in `file`.
 * @param ext Instance of an external reference resolver class.
 * @param options Optional parameters.
 *
 * @return A `ParsedList` containing a pointer to the root `Base` object.
 * Depending on `Provisioner_`, this may contain references to all nested objects. 
 * 
 * Any invalid representations in `contents` will cause an error to be thrown.
 */
template<class Provisioner_, class Externals_>
ParsedList parse(const std::string& file, const std::string& name, Externals_ ext, Options options = Options()) {
    H5::H5File handle(file, H5F_ACC_RDONLY);
    return parse<Provisioner_>(handle.openGroup(name), std::move(ext), options);
}

/**
 * Validate HDF5 file contents against the **uzuki2** specification, given the group handle.
 * Any invalid representations will cause an error to be thrown.
 *
 * @param handle Handle for a HDF5 group corresponding to the list.
 * @param num_external Expected number of external references. 
 * @param options Optional parameters.
 */
inline void validate(const H5::Group& handle, int num_external, const Options& options) {
    parse<DummyProvisioner>(handle, DummyExternals(num_external), options);
}

/**
 * Validate HDF5 file contents against the **uzuki2** specification, given the file path.
 * Any invalid representations will cause an error to be thrown.
 *
 * @param file Path to a HDF5 file.
 * @param name Name of the HDF5 group containing the list in `file`.
 * @param num_external Expected number of external references. 
 * @param options Optional parameters.
 */
inline void validate(const std::string& file, const std::string& name, int num_external, const Options& options) {
    parse<DummyProvisioner>(file, name, DummyExternals(num_external), options);
}

}

}

#endif
