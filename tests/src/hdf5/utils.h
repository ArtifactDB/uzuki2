#ifndef HDF5_UTILS_H
#define HDF5_UTILS_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <map>
#include <vector>
#include <string>
#include <type_traits>

#include "H5Cpp.h"

#include "uzuki2/uzuki2.hpp"

#include "../test_subclass.h"

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

inline void add_version(const H5::Group& parent, const std::string& version) {
    H5::StrType stype(0, H5T_VARIABLE);
    auto ahandle = parent.createAttribute("uzuki_version", stype, H5S_SCALAR);
    ahandle.write(stype, version);
}

template<typename T>
H5::DataSet write_number(const H5::Group& parent, const std::string& name, T value, const H5::DataType& dtype) {
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
H5::DataSet write_numbers(const H5::Group& parent, const std::string& name, const std::vector<T>& values, const H5::DataType& dtype, hsize_t chunk_size) {
    hsize_t len = values.size();
    H5::DataSpace dspace(1, &len);

    H5::DSetCreatPropList cplist;
    if (chunk_size) {
        cplist.setChunk(1, &chunk_size);
        cplist.setDeflate(8);
    }

    auto dhandle = parent.createDataSet(name, dtype, dspace, cplist);
    dhandle.write(values.data(), ritsuko::hdf5::as_numeric_datatype<T>());
    return dhandle;
}

template<typename T>
H5::DataSet write_numbers(const H5::Group& parent, const std::string& name, const std::vector<T>& values, const H5::DataType& dtype) {
    return write_numbers(parent, name, values, dtype, 0);
}

inline H5::DataSet write_string(const H5::Group& parent, const std::string& name, const std::string& value, bool variable) {
    H5::DataSpace dspace;
    if (variable) {
        H5::StrType stype(0, H5T_VARIABLE);
        auto dhandle = parent.createDataSet(name, stype, dspace);
        const auto ptr = value.c_str();
        dhandle.write(&ptr, stype);
        return dhandle;
    } else {
        H5::StrType stype(0, value.size());
        auto dhandle = parent.createDataSet(name, stype, dspace);
        dhandle.write(value.c_str(), stype);
        return dhandle;
    }
}

inline H5::DataSet write_string(const H5::Group& parent, const std::string& name, const std::string& value) {
    return write_string(parent, name, value, false);
}

inline H5::DataSet write_strings(const H5::Group& parent, const std::string& name, const std::vector<std::string>& values, bool variable, hsize_t chunk_size) {
    hsize_t len = values.size();
    H5::DataSpace dspace(1, &len);

    H5::DSetCreatPropList cplist;
    if (chunk_size) {
        cplist.setChunk(1, &chunk_size);
        cplist.setDeflate(6);
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

inline H5::DataSet write_strings(const H5::Group& parent, const std::string& name, const std::vector<std::string>& values) {
    return write_strings(parent, name, values, false, 0);
}

inline auto load_hdf5(const std::string& name, const std::string& group) {
    uzuki2::hdf5::Options opt;
    opt.strict_list = false;
    return uzuki2::hdf5::parse<DefaultProvisioner>(name, group, uzuki2::DummyExternals(), std::move(opt));
}

inline auto validate_hdf5(const std::string& name, const std::string& group) {
    uzuki2::hdf5::Options opt;
    opt.strict_list = false;
    return uzuki2::hdf5::validate(name, group, 0, std::move(opt));
}

inline auto load_hdf5_strict(const std::string& name, const std::string& group) {
    return uzuki2::hdf5::parse<DefaultProvisioner>(name, group, uzuki2::DummyExternals(), {});
}

inline void expect_hdf5_error(std::string file, std::string name, std::string msg) {
    std::string obs;
    try {
        uzuki2::hdf5::validate(file, name, 0, {});
    } catch (std::exception& e) {
        obs = e.what();
    }
    EXPECT_THAT(obs, ::testing::HasSubstr(msg));
}

#endif
