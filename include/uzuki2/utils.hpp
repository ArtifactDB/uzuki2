#ifndef UZUKI2_UTILS_HPP
#define UZUKI2_UTILS_HPP

#include <cctype>
#include <string>

namespace uzuki2 {

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

    if (val[8] == '3') {
        if (val[9] > '1') {
            return false;
        }
    } else if (val[8] > '3') {
        return false;
    }

    return true;
}

}

#endif
