#ifndef NUMERICAL_METHODS_COMMON_DEFINITIONS_H_
#define NUMERICAL_METHODS_COMMON_DEFINITIONS_H_

#include <cmath>
#include <limits>
#include <type_traits>

#include <glog/logging.h>

namespace numerical_methods {

template <typename Type>
inline constexpr Type getUndef() {
  return std::numeric_limits<Type>::quiet_NaN();
}

template <typename Type>
inline bool isUndef(Type val) {
  return std::isnan(val);
}

template <typename Type>
inline constexpr Type getInf() {
  return std::numeric_limits<Type>::infinity();
}

template <typename Type>
inline bool isInf(Type val) {
  return std::isinf(val);
}

template <typename Type>
inline constexpr Type getEps() {
  return std::numeric_limits<Type>::epsilon();
}

template <typename Type>
inline constexpr Type getPi() {
  return Type(4.0) * std::atan(Type(1.0));
}

} // namespace numerical_methods

#endif // NUMERICAL_METHODS_COMMON_DEFINITIONS_H_
