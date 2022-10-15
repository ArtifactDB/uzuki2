#ifndef UZUKI2_VALIDATE_HPP
#define UZUKI2_VALIDATE_HPP

#include "Dummy.hpp"
#include "parse.hpp"

#include <vector>
#include <algorithm>
#include <stdexcept>

/**
 * @file validate.hpp
 *
 * @brief Validate HDF5 file contents against the **uzuki2** spec.
 */

namespace uzuki2 {

/**
 * Validate HDF5 file contents against the **uzuki2** specification.
 * Any invalid representations will cause an error to be thrown.
 *
 * @param contents Handle to the relevant group of the HDF5 file.
 * @param num_external Expected number of external references. 
 */
inline void validate(const H5::Group& handle, const std::string& name, int num_external = 0) {
    DummyExternals others(num_external);
    parse<DummyProvisioner>(handle, name, others);
    return;
}

inline void validate(const std::string& file, const std::string& name, int num_external = 0) {
    DummyExternals others(num_external);
    parse<DummyProvisioner>(file, name, others);
    return;
}

}

#endif
