#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <type_traits>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "H5Cpp.h"

#include "uzuki2/uzuki2.hpp"

#include "test_subclass.h"

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

inline H5::Group nothing_opener(const H5::Group& parent, const std::string& name) {
    std::map<std::string, std::string> attrs;
    attrs["uzuki_object"] = "nothing";
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

inline void add_version(const H5::Group& parent, const std::string& version) {
    H5::StrType stype(0, H5T_VARIABLE);
    auto ahandle = parent.createAttribute("uzuki_version", stype, H5S_SCALAR);
    ahandle.write(stype, version);
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

inline H5::DataSet write_string(const H5::Group& parent, const std::string& name, const std::string& value) {
    H5::DataSpace dspace;
    H5::StrType stype(0, value.size());
    auto dhandle = parent.createDataSet(name, stype, dspace);
    dhandle.write(value.c_str(), stype);
    return dhandle;
}

template<typename T>
H5::DataSet create_dataset(const H5::Group& parent, const std::string& name, const std::vector<T>& values, const H5::DataType& dtype, bool compressed = false) {
    hsize_t len = values.size();
    H5::DataSpace dspace(1, &len);

    H5::DSetCreatPropList cplist;
    if (compressed) {
        hsize_t chunk = 57;
        cplist.setChunk(1, &chunk);
        cplist.setDeflate(8);
    } else {
        cplist = H5::DSetCreatPropList::DEFAULT;
    }

    auto dhandle = parent.createDataSet(name, dtype, dspace, cplist);
    dhandle.write(values.data(), ritsuko::hdf5::as_numeric_datatype<T>());
    return dhandle;
}

inline H5::DataSet create_dataset(const H5::Group& parent, const std::string& name, const std::vector<std::string>& values, bool variable = false, bool compressed = false) {
    hsize_t len = values.size();
    H5::DataSpace dspace(1, &len);

    H5::DSetCreatPropList cplist;
    if (compressed) {
        hsize_t chunk = 96;
        cplist.setChunk(1, &chunk);
        cplist.setDeflate(6);
    } else {
        cplist = H5::DSetCreatPropList::DEFAULT;
    }

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
        auto dhandle = parent.createDataSet(name, stype, dspace, cplist);
        dhandle.write(buffer.data(), stype);
        return dhandle;

    } else {
        std::vector<const char*> ptrs;
        ptrs.reserve(values.size());
        for (const auto& v : values) {
            ptrs.push_back(v.c_str());
        }

        H5::StrType stype(H5::PredType::C_S1, H5T_VARIABLE); 
        auto dhandle = parent.createDataSet(name, stype, dspace, cplist);
        dhandle.write(ptrs.data(), stype);
        return dhandle;
    }
}

inline auto load_hdf5(std::string name, std::string group) {
    uzuki2::hdf5::Options opt;
    opt.strict_list = false;
    return uzuki2::hdf5::parse<DefaultProvisioner>(name, group, std::move(opt));
}

inline auto load_hdf5_strict(std::string name, std::string group) {
    return uzuki2::hdf5::parse<DefaultProvisioner>(name, group);
}

inline auto load_json(std::string x, bool parallel = false) {
    uzuki2::json::Options opt;
    opt.parallel = parallel;
    opt.strict_list = false;
    return uzuki2::json::parse_buffer<DefaultProvisioner>(reinterpret_cast<const unsigned char*>(x.c_str()), x.size(), std::move(opt));
}

inline auto load_json_strict(std::string x, bool parallel = false) {
    uzuki2::json::Options opt;
    opt.parallel = parallel;
    return uzuki2::json::parse_buffer<DefaultProvisioner>(reinterpret_cast<const unsigned char*>(x.c_str()), x.size(), std::move(opt));
}

inline void expect_hdf5_error(std::string file, std::string name, std::string msg) {
    EXPECT_ANY_THROW({
        try {
            uzuki2::hdf5::validate(file, name);
        } catch (std::exception& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
            throw;
        }
    });
}

inline void expect_json_error(std::string json, std::string msg) {
    EXPECT_ANY_THROW({
        try {
            uzuki2::json::validate_buffer(reinterpret_cast<const unsigned char*>(json.c_str()), json.size());
        } catch (std::exception& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
            throw;
        }
    });
}

#endif
