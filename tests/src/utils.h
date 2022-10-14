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
    attrs["delayed_array"] = type;
    return super_group_opener(parent, name, attrs);
}

inline H5::Group null_opener(const H5::Group& parent, const std::string& name) {
    std::map<std::string, std::string> attrs;
    attrs["uzuki_object"] = "null";
    return super_group_opener(parent, name, attrs);
}

#endif

