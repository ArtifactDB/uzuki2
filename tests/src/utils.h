#ifndef UTILS_H
#define UTILS_H

#include "H5Cpp.h"
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <type_traits>

inline H5::Group super_group_opener(const H5::Group& parent, const std::string& name, const std::map<std::string, std::string>& attributes) {
    auto ghandle = parent.createGroup(name);
    for (const auto& p : attributes) {
        H5::StrType stype(0, H5T_VARIABLE);
        auto ahandle = ghandle.createAttribute(p.first, stype, H5S_SCALAR);
        ahandle.write(stype, p.second);
    }
    return ghandle;
}

inline H5::Group list_opener(const H5::Group& parent, const std::string& name) {
    std::map<std::string, std::string> attrs;
    attrs["uzuki_object"] = "list";
    return super_group_opener(parent, name, attrs);
}

inline H5::Group vector_opener(const H5::Group& parent, const std::string& name, const std::string& type) {
    std::map<std::string, std::string> attrs;
    attrs["uzuki_object"] = "vector";
    attrs["uzuki_type"] = type;
    return super_group_opener(parent, name, attrs);
}

inline H5::Group null_opener(const H5::Group& parent, const std::string& name) {
    std::map<std::string, std::string> attrs;
    attrs["uzuki_object"] = "null";
    return super_group_opener(parent, name, attrs);
}

inline H5::Group external_opener(const H5::Group& parent, const std::string& name) {
    std::map<std::string, std::string> attrs;
    attrs["uzuki_object"] = "external";
    return super_group_opener(parent, name, attrs);
}

inline H5::DataSet create_dataset(const H5::Group& parent, const std::string& name, hsize_t len, const H5::DataType& dtype) {
    H5::DataSpace dspace(1, &len);
    return parent.createDataSet(name, dtype, dspace);
}

template<typename T>
H5::DataSet write_scalar(const H5::Group& parent, const std::string& name, T value, const H5::DataType& dtype) {
    H5::DataSpace dspace;
    auto dhandle = parent.createDataSet(name, dtype, dspace);

    if constexpr(std::is_same<T, int>::value) {
        dhandle.write(&value, H5::PredType::NATIVE_INT);
    } else if constexpr(std::is_same<T, double>::value) {
        dhandle.write(&value, H5::PredType::NATIVE_DOUBLE);
    } else {
        throw std::runtime_error("unknown type!");
    }

    return dhandle;
}

template<typename T>
H5::DataSet create_dataset(const H5::Group& parent, const std::string& name, std::vector<T> values, const H5::DataType& dtype) {
    hsize_t len = values.size();
    H5::DataSpace dspace(1, &len);
    auto dhandle = parent.createDataSet(name, dtype, dspace);

    if constexpr(std::is_same<T, int>::value) {
        dhandle.write(values.data(), H5::PredType::NATIVE_INT);
    } else if constexpr(std::is_same<T, double>::value) {
        dhandle.write(values.data(), H5::PredType::NATIVE_DOUBLE);
    } else {
        throw std::runtime_error("unknown type!");
    }

    return dhandle;
}

inline H5::DataSet create_dataset(const H5::Group& parent, const std::string& name, std::vector<std::string> values, bool variable = false) {
    hsize_t len = values.size();
    H5::DataSpace dspace(1, &len);

    if (!variable) {
        size_t maxlen = 1;
        for (const auto& v : values) {
            if (v.size() > maxlen) {
                maxlen = v.size();
            }
        }

        std::vector<char> buffer(maxlen * values.size());
        for (size_t v = 0; v < values.size(); ++v) {
            const auto& current = values[v];
            std::copy(current.begin(), current.end(), buffer.data() + v * maxlen);
        }

        H5::StrType stype(0, maxlen);
        auto dhandle = parent.createDataSet(name, stype, dspace);
        dhandle.write(buffer.data(), stype);
        return dhandle;

    } else {
        std::vector<const char*> ptrs;
        ptrs.reserve(values.size());
        for (const auto& v : values) {
            ptrs.push_back(v.c_str());
        }

        H5::StrType stype(H5::PredType::C_S1, H5T_VARIABLE); 
        auto dhandle = parent.createDataSet(name, stype, dspace);
        dhandle.write(ptrs.data(), stype);
        return dhandle;
    }
}

#endif

