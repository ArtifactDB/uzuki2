#ifndef UZUKI2_PARSE_HPP
#define UZUKI2_PARSE_HPP

#include <memory>
#include <vector>
#include <cctype>
#include <string>
#include <stdexcept>

#include "H5Cpp.h"
#include "interfaces.hpp"

namespace uzuki2 {

/**
 * @cond
 */
inline std::string load_string_attribute(const H5::Attribute& attr, const std::string& field, const std::string& path) {
    if (attr.getTypeClass() != H5T_STRING || attr.getSpace().getSimpleExtentNdims() != 0) {
        throw std::runtime_error(std::string("'") + field + "' attribute should be a scalar string at '" + path + "'");
    }

    std::string output;
    attr.read(attr.getStrType(), output);
    return output;
}

template<class H5Object>
std::string load_string_attribute(const H5Object& handle, const std::string& field, const std::string& path) {
    if (!handle.attrExists(field)) {
        throw std::runtime_error(std::string("expected a '") + field + "' attribute at '" + path + "'");
    }
    return load_string_attribute(handle.openAttribute(field), field, path);
}

template<class Function>
void load_string_dataset(const H5::DataSet& handle, hsize_t full_length, const std::string& path, Function fun) {
    auto dtype = handle.getDataType();

    // TODO: read this in chunks.
    if (dtype.isVariableStr()) {
        std::vector<char*> buffer(full_length);
        handle.read(buffer.data(), dtype);

        for (size_t i = 0; i < full_length; ++i) {
            fun(i, std::string(buffer[i]));
        }

        auto dspace = handle.getSpace();
        H5Dvlen_reclaim(dtype.getId(), dspace.getId(), H5P_DEFAULT, buffer.data());

    } else {
        size_t len = dtype.getSize();
        std::vector<char> buffer(len * full_length);
        handle.read(buffer.data(), dtype);

        auto start = buffer.data();
        for (size_t i = 0; i < full_length; ++i, start += len) {
            size_t j = 0;
            for (; j < len && start[j] != '\0'; ++j) {}
            fun(i, std::string(start, start + j));
        }
    }
}

inline hsize_t check_1d_length(const H5::DataSet& handle, const std::string& path) {
    auto dspace = handle.getSpace();
    int ndims = dspace.getSimpleExtentNdims();
    if (ndims != 1) {
        throw std::runtime_error("expected a 1-dimensional dataset at '" + path + "'");
    }
    hsize_t dims;
    dspace.getSimpleExtentDims(&dims);
    return dims;
}

inline bool is_date(const std::string& val) {
    if (val.size() != 10) {
        return false;
    } 
    
    for (size_t p = 0; p < val.size(); ++p) {
        if (p == 4 || p == 7) {
            if (val[p] != '-') {
                return false;
            }
        } else {
            if (!std::isdigit(val[p])) {
                return false;
            }
        }
    }

    if (val[5] == '1') {
        if (val[6] > '2') {
            return false;
        }
    } else if (val[5] != '0') {
        return false;
    }

    return true;
}

template<class Host, class Function>
void parse_integer_like(H5::DataSet handle, Host* ptr, const std::string& path, Function check) {
    if (handle.getDataType().getClass() != H5T_INTEGER) {
        throw std::runtime_error("expected an integer dataset at '" + path + "'");
    }

    bool has_missing = handle.attrExists("uzuki_missing");
    int missing_val = 0;
    if (has_missing) {
        auto attr = handle.openAttribute("uzuki_missing");
        if (attr.getTypeClass() != H5T_INTEGER || attr.getSpace().getSimpleExtentNdims() != 0) {
            throw std::runtime_error("'uzuki_missing' attribute should be a scalar integer at '" + path + "'");
        }
        attr.read(H5::PredType::NATIVE_INT, &missing_val);
    }

    size_t len = ptr->size();

    // TODO: loop in chunks to reduce memory usage.
    std::vector<int> buffer(len);
    handle.read(buffer.data(), H5::PredType::NATIVE_INT);

    for (hsize_t i = 0; i < len; ++i) {
        auto current = buffer[i];
        if (has_missing && missing_val == current) {
            ptr->set_missing(i);
        } else {
            check(current);
            ptr->set(i, current);
        }
    }
}

template<class Host, class Function>
void parse_string_like(H5::DataSet handle, Host* ptr, const std::string& path, Function check) {
    auto dtype = handle.getDataType();
    if (dtype.getClass() != H5T_STRING) {
        throw std::runtime_error("expected a string dataset at '" + path + "'");
    }

    bool has_missing = handle.attrExists("uzuki_missing");
    std::string missing_val = 0;
    if (has_missing) {
        missing_val = load_string_attribute(handle.openAttribute("uzuki_missing"), "uzuki_missing", path);
    }

    load_string_dataset(handle, ptr->size(), path, [&](size_t i, std::string x) -> void {
        if (has_missing && x == missing_val) {
            ptr->set_missing(i);
        } else {
            check(x);
            ptr->set(i, x);
        }
    });
}

template<class Host, class Function>
void parse_floats(H5::DataSet handle, Host* ptr, const std::string& path, Function check) {
    if (handle.getDataType().getClass() != H5T_FLOAT) {
        throw std::runtime_error("expected a float dataset at '" + path + "'");
    }

    bool has_missing = handle.attrExists("uzuki_missing");
    double missing_val; 
    if (has_missing) {
        auto attr = handle.openAttribute("uzuki_missing");
        if (attr.getDataType().getClass() != H5T_FLOAT || attr.getSpace().getSimpleExtentNdims() != 0) {
            throw std::runtime_error("'usuki_missing' attribute should be a scalar float at '" + path + "'");
        }
        attr.read(H5::PredType::NATIVE_DOUBLE, &missing_val);
    }

    size_t len = ptr->size();

    // TODO: loop in chunks to reduce memory usage.
    std::vector<double> buffer(len);
    handle.read(buffer.data(), H5::PredType::NATIVE_DOUBLE);

    for (hsize_t i = 0; i < len; ++i) {
        auto current = buffer[i];
        if (has_missing && missing_val == current) {
            ptr->set_missing(i);
        } else {
            check(current);
            ptr->set(i, current);
        }
    }
}

template<class Host>
void parse_names(H5::Group handle, Host* ptr, const std::string& path, const std::string& dpath) {
    if (handle.exists("names")) {
        auto npath = path + "/names";
        if (handle.childObjType("names") != H5O_TYPE_DATASET) {
            throw std::runtime_error("expected a dataset at '" + npath + "'");
        }
        ptr->use_names();

        auto nhandle = handle.openDataSet("names");
        auto dtype = nhandle.getDataType();
        if (dtype.getClass() != H5T_STRING) {
            throw std::runtime_error("expected a string dataset at '" + npath + "'");
        }

        auto len = ptr->size();
        auto nlen = check_1d_length(nhandle, npath);
        if (nlen != len) {
            throw std::runtime_error("length of '" + npath + "' should be equal to length of '" + dpath + "'");
        }

        load_string_dataset(nhandle, nlen, npath, [&](size_t i, std::string x) -> void { ptr->set_name(i, x); });
    }
}
/**
 * @endcond
 */

template<class Provisioner, class Externals>
std::shared_ptr<Base> parse(const H5::Group& handle, Externals& ext, const std::string& path) {
    // Deciding what type we're dealing with.
    auto object_type = load_string_attribute(handle, "uzuki_object", path);
    std::shared_ptr<Base> output;

    if (object_type == "list") {
        auto dpath = path + "/data";
        if (!handle.exists("data") || handle.childObjType("data") != H5O_TYPE_GROUP) {
            throw std::runtime_error("expected a group at '" + dpath);
        }
        auto dhandle = handle.openGroup("data");
        auto len = dhandle.getNumObjs();
        auto lptr = Provisioner::new_List(len);
        output.reset(lptr);

        for (int i = 0; i < len; ++i) {
            auto istr = std::to_string(i);
            auto ipath = dpath + "/" + istr;
            if (!dhandle.exists(istr) || dhandle.childObjType(istr) != H5O_TYPE_GROUP) {
                throw std::runtime_error("expected a group at '" + ipath + "'");
            }
            auto lhandle = dhandle.openGroup(istr);
            lptr->set(i, parse<Provisioner>(lhandle, ext, dpath));
        }

        parse_names(handle, lptr, path, dpath);

    } else if (object_type == "vector") {
        auto vector_type = load_string_attribute(handle, "uzuki_object", path);

        auto dpath = path + "/data";
        if (!handle.exists("data") || handle.childObjType("data") != H5O_TYPE_DATASET) {
            throw std::runtime_error("expected a dataset at '" + dpath + "'");
        }
        auto dhandle = handle.openDataSet("data");
        auto dspace = dhandle.getSpace();
        int ndims = dspace.getSimpleExtentNdims();
        if (ndims !=  0) {
            throw std::runtime_error("expected 1-dimensional dataset at '" + dpath + "'");
        } 
        hsize_t len;
        dspace.getSimpleExtentDims(&len);

        if (vector_type == "integer") {
            auto iptr = Provisioner::new_Integer(len);
            output.reset(iptr);
            parse_integer_like(dhandle, iptr, dpath, [](int x) -> void {});

        } else if (vector_type == "boolean") {
            auto bptr = Provisioner::new_Boolean(len);
            output.reset(bptr);
            parse_integer_like(dhandle, bptr, dpath, [&](int x) -> void { 
                if (x != 0 && x != 1) {
                     throw std::runtime_error("boolean values should be 0 or 1 in '" + dpath + "'");
                }
            });

        } else if (vector_type == "factor" || vector_type == "ordered") {
            // First we need to figure out the number of levels.
            auto levpath = path + "/levels";
            if (!handle.exists("levels") || handle.childObjType("levels") != H5O_TYPE_DATASET) {
                throw std::runtime_error("expected a dataset at '" + levpath + "'");
            }
            auto levhandle = handle.openDataSet("levels");
            auto levtype = levhandle.getDataType();
            if (levtype.getClass() != H5T_STRING) {
                throw std::runtime_error("expected a string dataset at '" + levpath + "'");
            }
            auto levlen = check_1d_length(levhandle, levpath);

            // Then we can initialize the interface.
            auto fptr = Provisioner::new_Factor(len, levlen);
            output.reset(fptr);
            if (vector_type == "ordered") {
                fptr->is_ordered();
            }

            parse_integer_like(dhandle, fptr, dpath, [&](int x) -> void { 
                if (x < 0 || x >= levlen) {
                     throw std::runtime_error("factor codes should be non-negative and less than the number of levels in '" + dpath + "'");
                }
            });

            load_string_dataset(levhandle, levlen, levpath, [&](size_t i, std::string x) -> void { fptr->set_level(i, x); });

        } else if (vector_type == "string") {
            auto sptr = Provisioner::new_String(len);
            output.reset(sptr);
            parse_string_like(dhandle, sptr, dpath, [](const std::string& x) -> void {});

        } else if (vector_type == "date") {
            auto dptr = Provisioner::new_Date(len);
            output.reset(dptr);
            parse_string_like(dhandle, dptr, dpath, [&](const std::string& x) -> void {
                if (!is_date(x)) {
                     throw std::runtime_error("dates should follow YYYY-MM-DD formatting in '" + dpath + "'");
                }
            });

        } else if (vector_type == "float") {
            auto dptr = Provisioner::new_Double(len);
            output.reset(dptr);
            parse_floats(dhandle, dptr, dpath, [](double x) -> void {});

        } else {
            throw std::runtime_error("unknown vector type '" + vector_type + "' for '" + path + "'");
        }

        auto vptr = static_cast<Vector*>(output.get());
        parse_names(handle, vptr, path, dpath);

    } else if (object_type == "null") {
        output.reset(Provisioner::new_Nothing());

    } else if (object_type == "other") {
        auto ipath = path + "/index";
        if (!handle.exists("index") || handle.childObjType("index") != H5O_TYPE_DATASET) {
            throw std::runtime_error("expected a dataset at '" + ipath + "'");
        }
        auto ihandle = handle.openDataSet("index");

        auto ispace = ihandle.getSpace();
        int idims = ispace.getSimpleExtentNdims();
        if (idims != 1) {
            throw std::runtime_error("expected scalar dataset at '" + ipath + "'");
        } 

        int idx;
        ihandle.read(&idx, H5::PredType::NATIVE_INT);
        if (idx < 0 || static_cast<size_t>(idx) >= ext.size()) {
            throw std::runtime_error("external index out of range at '" + ipath + "'");
        }

        output.reset(Provisioner::new_Other(ext.get(idx)));
    }

    return output;
}

template<class Provisioner, class Externals>
std::shared_ptr<Base> parse(const std::string& file, Externals& ext, const std::string& path) {
    H5::H5File handle(file, H5F_ACC_RDONLY);
    return parse<Provisioner>(handle.openGroup(path), ext, path);
}

}

#endif
