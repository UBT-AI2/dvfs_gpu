#ifndef NUMERICAL_METHODS_OPTIMIZATION_OPTIMIZATION_METHOD_H_
#define NUMERICAL_METHODS_OPTIMIZATION_OPTIMIZATION_METHOD_H_

#include <type_traits>

#include <Eigen/Dense>
#include <glog/logging.h>

#include "numerical-methods/common-definitions.h"

namespace numerical_methods {

template <typename Type, int Size>
class OptimizationMethod {
public:
  
  typedef Type type;
  static constexpr int size = Size;
  
  template <bool Static = Size != Eigen::Dynamic>
  OptimizationMethod(typename std::enable_if<Static>::type* = nullptr) : 
      dimension_(Size) {}
  template <bool Dynamic = Size == Eigen::Dynamic>
  explicit OptimizationMethod(int dimension, 
      typename std::enable_if<Dynamic>::type* = nullptr) : 
      dimension_(dimension) {
    CHECK_GT(dimension, 0) << "Dimension must be positive.";
  }
  virtual ~OptimizationMethod() {}
  
  inline int getDimension() const {
    return dimension_;
  }
  
  class Options {
  public:
    Options() : min_iterations_(1), 
                max_iterations_(100), 
                param_tolerance_(1.0e-6), 
                func_tolerance_(1.0e-3) {}
    inline int getMinIterations() const {
      return min_iterations_;
    }
    inline int getMaxIterations() const {
      return max_iterations_;
    }
    inline Type getParamTolerance() const {
      return param_tolerance_;
    }
    inline Type getFuncTolerance() const {
      return func_tolerance_;
    }
    inline void setMinIterations(int min_iterations) {
      CHECK_GT(min_iterations, 0) 
          << "Minimum number of iterations must be positive.";
      min_iterations_ = min_iterations;
    }
    inline void setMaxIterations(int max_iterations) {
      CHECK_GT(max_iterations, 0) 
          << "Maximum number of iterations must be positive.";
      max_iterations_ = max_iterations;
    }
    inline void setParamTolerance(Type param_tolerance) {
      CHECK_GT(param_tolerance, Type(0.0)) 
          << "Parameter tolerance must be positive.";
      param_tolerance_ = param_tolerance;
    }
    inline void setFuncTolerance(Type func_tolerance) {
      CHECK_GT(func_tolerance, Type(0.0)) 
          << "Function tolerance must be positive.";
      func_tolerance_ = func_tolerance;
    }
  private:
    int min_iterations_;
    int max_iterations_;
    Type param_tolerance_;
    Type func_tolerance_;
  } options;
  
  // Find minimum of function.
  template <class Function>
  Eigen::Matrix<Type, Size, 1> minimize(const Function& function, 
      const Eigen::Matrix<Type, Size, 1>& point) const {}
  
private:
  const int dimension_;
  
}; // OptimizationMethod

} // namespace numerical_methods

#endif // NUMERICAL_METHODS_OPTIMIZATION_OPTIMIZATION_METHOD_H_
