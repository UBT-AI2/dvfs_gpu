#ifndef NUMERICAL_METHODS_OPTIMIZATION_NELDER_MEAD_METHOD_H_
#define NUMERICAL_METHODS_OPTIMIZATION_NELDER_MEAD_METHOD_H_

#include <cmath>
#include <limits>
#include <type_traits>
#include <utility>

#include <Eigen/Dense>
#include <glog/logging.h>

#include "numerical-methods/common-definitions.h"
#include "numerical-methods/optimization/optimization-method.h"

namespace numerical_methods {

// This class implements multi-dimensional minimization by the downhill simplex 
// search method of Nelder and Mead (1965), as implemented by O'Neill (1971), 
// with subsequent comments by Chambers and Ertel (1974), Benyon (1976) and 
// Hill (1978).
// 
// J. A. Nelder and R. Mead, "A Simplex Method for Function Minimization," in 
// The Computer Journal, vol. 7, pp. 308–313 (1965).
// 
// R. O'Neill, "Statistical Algorithms - Algorithm AS 47: Function Minimization 
// using a Simplex Procedure," in Applied Statistics, vol. 20, no. 3, 
// pp. 338-345 (1971).
// 
// J. M. Chambers and J. E. Ertel, "Statistical Algorithms - Remark AS R11: A 
// Remark on Algorithm AS 47: Function Minimization using a Simplex Procedure," 
// in Applied Statistics, vol. 23, no. 2, pp. 250-251 (1974).
// 
// P. R. Benyon, "Statistical Algorithms - Remark AS R15: Function Minimization 
// using a Simplex Procedure," in Applied Statistics, vol. 25, no. 1, p. 97 
// (1976).
// 
// I. D. Hill, "Remark AS R28: A Remark on Algorithm AS 47: Function 
// Minimization using a Simplex Procedure," in Journal of the Royal Statistical 
// Society, Series C (Applied Statistics), vol. 27, no. 3, pp. 380-382 (1978).
template <typename Type, int Size>
class NelderMeadMethod : public OptimizationMethod<Type, Size> {
public:
  
  template <bool Static = Size != Eigen::Dynamic>
  NelderMeadMethod(typename std::enable_if<Static>::type* = nullptr) : 
      OptimizationMethod<Type, Size>() {}
  template <bool Dynamic = Size == Eigen::Dynamic>
  explicit NelderMeadMethod(int dimension, 
      typename std::enable_if<Dynamic>::type* = nullptr) : 
      OptimizationMethod<Type, Size>(dimension) {}
  
  // Search for the minimum of a function from a given initial guess.
  template <class Function>
  Eigen::Matrix<Type, Size, 1> minimize(const Function& function, 
      const Eigen::Matrix<Type, Size, 1>& point) const {
    
    const int dimension = this->getDimension();
    
    typename std::conditional<Size != Eigen::Dynamic, 
        Eigen::Matrix<Type, Size, Size + 1>, 
        Eigen::Matrix<Type, Size, Size>>::type 
            points(dimension, dimension + 1);
    
    typename std::conditional<Size != Eigen::Dynamic, 
        Eigen::Matrix<Type, Size + 1, 1>, 
        Eigen::Matrix<Type, Size, 1>>::type values(dimension + 1);
    
    // Initialize simplex.
    const std::pair<Type, Type> scale = this->options.getInitScale();
    points.col(0) = point;
    values(0) = function(point);
    for (std::size_t k = 1; k <= dimension; ++k) {
      points.col(k) = point;
      points(k - 1, k) += (point(k - 1) != 0.0) ? 
          (scale.first * point(k - 1)) : (scale.second);
      values(k) = function(points.col(k));
    }
    
    std::vector<std::size_t> ind(dimension + 1);
    for (std::size_t k = 0; k <= dimension; ++k) {
      ind[k] = k;
    }
    
    Eigen::Matrix<Type, Size, 1> centroid(dimension);
    
    bool is_converged = false;
    for (int iter = 0; iter < this->options.getMaxIterations(); ++iter) {
      
      // Sort points in simplex according to value.
      std::sort(ind.begin(), ind.end(), 
          [&values](std::size_t i, std::size_t j) {
            return values(i) < values(j);
          });
      
      // Store indices to best and worst points.
      const std::size_t i = ind[0];
      const std::size_t j = ind[dimension];
      
      if (iter > this->options.getMinIterations()) {
        
        Type point_difference = 0.0;
        Type value_difference = 0.0;
        for (std::size_t k = 1; k <= dimension; ++k) {
          point_difference = std::max(point_difference, 
              (points.col(ind[k]) - points.col(i)).norm());
          value_difference = std::max(value_difference, 
              std::abs(values(ind[k]) - values(i)));
        }
        
        // Check convergence.
        is_converged = 
            (point_difference <= std::max(this->options.getParamTolerance(), 
             std::numeric_limits<Type>::epsilon() * points.col(i).norm()))
            && (value_difference <= std::max(this->options.getFuncTolerance(), 
                std::numeric_limits<Type>::epsilon() * std::abs(values(i))));
        if (is_converged) {
          break;
        }
        
      }
      
      // Compute centroid.
      centroid.setZero();
      for (std::size_t k = 0; k <= dimension; ++k) {
        if (k != j) {
          centroid += points.col(k);
        }
      }
      centroid /= static_cast<Type>(dimension);
      
      // Perform reflection.
      const Eigen::Matrix<Type, Size, 1> reflection_point = centroid 
          + reflection_coefficient_ * (centroid - points.col(j));
      const Type reflection_value = function(reflection_point);
      if (reflection_value < values(i)) {
        
        // Perform expansion.
        const Eigen::Matrix<Type, Size, 1> expansion_point = centroid 
            + expansion_coefficient_ * (centroid - points.col(j));
        const Type expansion_value = function(expansion_point);
        if (expansion_value < reflection_value) {
          points.col(j) = expansion_point;
          values(j) = expansion_value;
        } else {
          points.col(j) = reflection_point;
          values(j) = reflection_value;
        }
        
      } else {
        if (reflection_value < values(ind[dimension - 1])) {
          points.col(j) = reflection_point;
          values(j) = reflection_value;
        } else {
          bool shrink = false;
          if (reflection_value < values(j)) {
            
            // Perform outside contraction.
            const Eigen::Matrix<Type, Size, 1> contraction_point = centroid 
                + contraction_coefficient_ * (centroid - points.col(j));
            const Type contraction_value = function(contraction_point);
            if (contraction_value < reflection_value) {
              points.col(j) = contraction_point;
              values(j) = contraction_value;
            } else {
              shrink = true;
            }
            
          } else {
            
            // Perform inside contraction.
            const Eigen::Matrix<Type, Size, 1> contraction_point = centroid 
                - contraction_coefficient_ * (centroid - points.col(j));
            const Type contraction_value = function(contraction_point);
            if (contraction_value < values(j)) {
              points.col(j) = contraction_point;
              values(j) = contraction_value;
            } else {
              shrink = true;
            }
            
          }
          if (shrink) {
            
            // Perform shrinkage.
            for (std::size_t k = 0; k <= dimension; ++k) {
              if (k != i) {
                points.col(k) = points.col(i) 
                    + shrinkage_coefficient_ * (points.col(k) - points.col(i));
                values(k) = function(points.col(k));
              }
            }
            
          }
        }
        
      }
      
    }
    
    LOG_IF(WARNING, !is_converged) 
        << "Maximum number of iterations reached. Results may be inaccurate.";
    return centroid;
    
  }
  
  class Options : public OptimizationMethod<Type, Size>::Options {
  public:
    Options() : OptimizationMethod<Type, Size>::Options(), 
        init_scale_(std::make_pair(5.0e-2, 0.25e-3)) {}
    inline const std::pair<Type, Type>& getInitScale() const {
      return init_scale_;
    }
    inline void setInitScale(const std::pair<Type, Type>& init_scale) {
      CHECK_GT(std::min(init_scale.first, init_scale.second), 0.0) 
          << "Initial scale must be positive.";
      init_scale_ = init_scale;
    }
  private:
    std::pair<Type, Type> init_scale_;
  } options;
  
private:
  
  // Simplex coefficients.
  static constexpr Type reflection_coefficient_ = 1.0;
  static constexpr Type expansion_coefficient_= 2.0;
  static constexpr Type contraction_coefficient_ = - 0.5;
  static constexpr Type shrinkage_coefficient_ = 0.5;
  
}; // NelderMeadMethod

template <typename Type, int Size> 
constexpr Type NelderMeadMethod<Type, Size>::reflection_coefficient_;
template <typename Type, int Size> 
constexpr Type NelderMeadMethod<Type, Size>::expansion_coefficient_;
template <typename Type, int Size> 
constexpr Type NelderMeadMethod<Type, Size>::contraction_coefficient_;
template <typename Type, int Size> 
constexpr Type NelderMeadMethod<Type, Size>::shrinkage_coefficient_;

} // namespace numerical_methods

#endif // NUMERICAL_METHODS_OPTIMIZATION_NELDER_MEAD_METHOD_H_
