#ifndef UZUKI2_UTILS_HPP
#define UZUKI2_UTILS_HPP

#include <type_traits>

namespace uzuki2 {

template<typename Input_>
using I = std::remove_cv_t<std::remove_reference_t<Input_> >;

}

#endif
