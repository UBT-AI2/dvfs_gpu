#ifndef NUMERICAL_METHODS_UTILITY_LINEAR_MODEL_H_
#define NUMERICAL_METHODS_UTILITY_LINEAR_MODEL_H_

#include <type_traits>

#include <Eigen/Dense>
#include <glog/logging.h>

#include "numerical-methods/common-definitions.h"
#include "numerical-methods/utility/incremental-statistics.h"

namespace numerical_methods {

template <typename Type, int Size>
class LinearModel {
public:
  
  typedef Type type;
  static constexpr int size = Size;
  
  template <bool Static = Size != Eigen::Dynamic>
  LinearModel(
      typename std::enable_if<Static>::type* = nullptr) : 
      dimension_(Size), 
      discount_(0.0), 
      regularizer_(0.0), 
      bias_(0.0), 
      valid_(true) {
    coefficients_.setZero();
  }
  template <bool Static = Size != Eigen::Dynamic>
  explicit LinearModel(Type discount, 
      typename std::enable_if<Static>::type* = nullptr) : 
      dimension_(Size), 
      discount_(discount), 
      regularizer_(0.0), 
      statistics_(discount), 
      coefficients_(Size), 
      bias_(0.0), 
      valid_(true) {
    CHECK_GE(discount, 0.0) << "Discount factor must be non-negative.";
    CHECK_LE(discount, 1.0) << "Discount factor must not exceed one.";
    coefficients_.setZero();
  }
  template <bool Static = Size != Eigen::Dynamic>
  LinearModel(Type discount, Type regularizer, 
      typename std::enable_if<Static>::type* = nullptr) : 
      dimension_(Size), 
      discount_(discount), 
      regularizer_(regularizer), 
      statistics_(discount), 
      coefficients_(Size), 
      bias_(0.0), 
      valid_(true) {
    CHECK_GE(discount, 0.0) << "Discount factor must be non-negative.";
    CHECK_LE(discount, 1.0) << "Discount factor must not exceed one.";
    CHECK_GE(regularizer, 0.0) << "Regularizer must be non-negative.";
    coefficients_.setZero();
  }
  template <bool Dynamic = Size == Eigen::Dynamic>
  explicit LinearModel(int dimension, 
      typename std::enable_if<Dynamic>::type* = nullptr) : 
      dimension_(dimension), 
      discount_(0.0), 
      regularizer_(0.0), 
      statistics_(dimension + 1), 
      coefficients_(dimension), 
      bias_(0.0), 
      valid_(true) {
    CHECK_GT(dimension, 0) << "Dimension must be positive.";
    coefficients_.setZero();
  }
  template <bool Dynamic = Size == Eigen::Dynamic>
  LinearModel(int dimension, Type discount, 
      typename std::enable_if<Dynamic>::type* = nullptr) : 
      dimension_(dimension), 
      discount_(discount), 
      regularizer_(0.0), 
      statistics_(dimension + 1, discount), 
      coefficients_(dimension), 
      bias_(0.0), 
      valid_(true) {
    CHECK_GT(dimension, 0) << "Dimension must be positive.";
    CHECK_GE(discount, 0.0) << "Discount factor must be non-negative.";
    CHECK_LE(discount, 1.0) << "Discount factor must not exceed one.";
    coefficients_.setZero();
  }
  template <bool Dynamic = Size == Eigen::Dynamic>
  LinearModel(int dimension, Type discount, Type regularizer, 
      typename std::enable_if<Dynamic>::type* = nullptr) : 
      dimension_(dimension), 
      discount_(discount), 
      regularizer_(regularizer), 
      statistics_(dimension + 1, discount), 
      coefficients_(dimension), 
      bias_(0.0), 
      valid_(true) {
    CHECK_GT(dimension, 0) << "Dimension must be positive.";
    CHECK_GE(discount, 0.0) << "Discount factor must be non-negative.";
    CHECK_LE(discount, 1.0) << "Discount factor must not exceed one.";
    CHECK_GE(regularizer, 0.0) << "Regularizer must be non-negative.";
    coefficients_.setZero();
  }
  virtual ~LinearModel() {}
  
  inline int getDimension() const {
    return dimension_;
  }
  inline Type getDiscount() const {
    return discount_;
  }
  inline Type getRegularizer() const {
    return regularizer_;
  }
  
  const typename std::conditional<Size != Eigen::Dynamic, 
      IncrementalStatistics<Type, Size + 1>, 
      IncrementalStatistics<Type, Size>>::type& getStatistics() const {
    return statistics_;
  }
  
  inline const Eigen::Matrix<Type, Size, 1>& getCoefficients() const {
    if (!valid_) {
      evaluate();
      valid_ = true;
    }
    return coefficients_;
  }
  inline Type getBias() const {
    if (!valid_) {
      evaluate();
      valid_ = true;
    }
    return bias_;
  }
  
  // Reset the model.
  void clear() {
    statistics_.clear();
    coefficients_.setZero();
    bias_ = 0.0;
    valid_ = true;
  }
  
  // Incrementally update statistics as new data become available.
  void update(const Eigen::Matrix<Type, Size, 1>& predictors, Type response, 
      Type weight) {
    
    CHECK_EQ(predictors.size(), dimension_) 
        << "Predictors have the wrong size.";
    
    // Concatenate predictors and response.
    typename std::conditional<Size != Eigen::Dynamic, 
        Eigen::Matrix<Type, Size + 1, 1>, 
        Eigen::Matrix<Type, Size, 1>>::type point(dimension_ + 1);
    point.head(dimension_) = predictors;
    point(dimension_) = response;
    
    // Update sufficient statistics.
    statistics_.update(point, weight);
    
    valid_ = false;
    
  }
  
  // Incrementally update statistics with default weight.
  void update(const Eigen::Matrix<Type, Size, 1>& predictors, Type response) {
    update(predictors, response, 1.0);
  }
  
private:
  
  // Model size and parameters.
  const int dimension_;
  const Type discount_;
  const Type regularizer_;
  
  // Sufficient statistics.
  typename std::conditional<Size != Eigen::Dynamic, 
      IncrementalStatistics<Type, Size + 1>, 
      IncrementalStatistics<Type, Size>>::type statistics_;
  
  // Regression coefficients and bias term.
  mutable Eigen::Matrix<Type, Size, 1> coefficients_;
  mutable Type bias_;
  
  mutable bool valid_;
  
  void evaluate() const {
    
    const typename std::conditional<Size != Eigen::Dynamic, 
        Eigen::Matrix<Type, Size + 1, 1>, 
        Eigen::Matrix<Type, Size, 1>>::type& mean = statistics_.getMean();
    const typename std::conditional<Size != Eigen::Dynamic, 
        Eigen::Matrix<Type, Size + 1, Size + 1>, 
        Eigen::Matrix<Type, Size, Size>>::type& 
        covariance = statistics_.getCovariance();
    
    // Update regression coefficients based on sufficient statistics.
    coefficients_ = (covariance.topLeftCorner(dimension_, dimension_) + 
        regularizer_ * Eigen::Matrix<Type, Size, Size>::Identity(dimension_, 
            dimension_)).ldlt().solve(covariance.topRightCorner(dimension_, 1));
    
    // Update bias term.
    bias_ = mean(dimension_) - mean.head(dimension_).dot(coefficients_);
    
  }
  
}; // LinearModel

} // numerical_methods

#endif // NUMERICAL_METHODS_UTILITY_LINEAR_MODEL_H_
