#include <cmath>
#include <limits>
#include <random>
#include <utility>
#include <vector>

#include <Eigen/Dense>
#include <gtest/gtest.h>

#include "numerical-methods/common-definitions.h"
#include "numerical-methods/utility/linear-model.h"

namespace numerical_methods {

// Sample for testing linear model.
template <typename Type, int Size>
class TestSample {
public:
  TestSample(int dimension, int size) : dimension_(dimension), size_(size) {
    if (Size != Eigen::Dynamic) {
      CHECK_EQ(dimension, Size) << "Dimension must be consistent.";
    } else {
      CHECK_GT(dimension, 0) << "Dimension must be positive.";
    }
    CHECK_GT(size, 0) << "Size must be positive.";
    generate();
  }
  inline int getDimension() const {
    return dimension_;
  }
  inline int getSize() const {
    return size_;
  }
  inline Eigen::Matrix<Type, Size, 1> getPredictors(std::size_t i) const {
    CHECK_LT(i, size_);
    return predictors_.col(i);
  }
  inline Type getResponse(std::size_t i) const {
    CHECK_LT(i, size_);
    return responses_(i);
  }
  inline Type getWeight(std::size_t i) const {
    CHECK_LT(i, size_);
    return weights_(i);
  }
  std::pair<Eigen::Matrix<Type, Size, 1>, Type> fitParameters(Type discount, 
      Type regularizer) {
    
    CHECK_GE(discount, 0.0) << "Discount must be non-negative.";
    CHECK_LE(discount, 1.0) << "Discount must be less than or equal to one.";
    CHECK_GE(regularizer, 0.0) << "Regularizer must be non-negative.";
    
    Eigen::Matrix<Type, Size, 1> mean(dimension_);
    Eigen::Matrix<Type, Size, Size> covariance(dimension_, dimension_);
    Eigen::Matrix<Type, Size, 1> cross_covariance(dimension_);
    Type average = 0;
    
    // Compute mean.
    mean.setZero();
    Type sum = 0.0;
    for (int i = 0; i < size_; ++i) {
      const Type weight = weights_(i) * std::pow(1.0 - discount, size_ - 1 - i);
      mean += weight * predictors_.col(i);
      average += weight * responses_(i);
      sum += weight;
    }
    mean /= sum;
    average /= sum;
    
    // Compute covariance and cross-covariance.
    covariance.setZero();
    cross_covariance.setZero();
    for (int i = 0; i < size_; ++i) {
      const Type weight = weights_(i) * std::pow(1.0 - discount, size_ - 1 - i);
      const Eigen::Matrix<Type, Size, 1> residual = predictors_.col(i) - mean;
      covariance += weight * residual * residual.transpose();
      cross_covariance += (weight * (responses_(i) - average)) * residual;
    }
    covariance /= sum;
    cross_covariance /= sum;
    
    // Fit regression coefficients.
    const Eigen::Matrix<Type, Size, 1> coefficients = (covariance + 
        regularizer * Eigen::Matrix<Type, Size, Size>::Identity(dimension_, 
        dimension_)).ldlt().solve(cross_covariance);
    
    // Fit bias term.
    const Type bias = average - mean.dot(coefficients);
    
    return std::make_pair(coefficients, bias);
    
  }
private:
  const int dimension_;
  const int size_;
  Eigen::Matrix<Type, Size, Eigen::Dynamic> predictors_;
  Eigen::Matrix<Type, Eigen::Dynamic, 1> responses_;
  Eigen::Matrix<Type, Eigen::Dynamic, 1> weights_;
  void generate() {
    
    std::uniform_real_distribution<Type> scale_distribution(1.0, 5.0);
    std::uniform_real_distribution<Type> strength_distribution(1.0, 2.0);
    std::uniform_real_distribution<Type> bias_distribution(- 1.0, 1.0);
    std::normal_distribution<Type> noise_distribution(0.0, 1.0);
    std::uniform_real_distribution<Type> weight_distribution(0.0, 1.0);
    
    std::mt19937 engine;
    std::random_device device;
    engine.seed(device());
    
    // Generate random scale and strength parameters.
    const Type scale = scale_distribution(engine);
    const Type strength = strength_distribution(engine);
    
    // Generate random regression coefficients.
    const Eigen::Matrix<Type, Size, 1> coefficients = 
        strength * Eigen::Matrix<Type, Size, 1>::Random(dimension_, 1);
    const Type bias = bias_distribution(engine);
    
    predictors_.resize(dimension_, size_);
    responses_.resize(size_);
    weights_.resize(size_);
    
    // Generate sample from stochastic process.
    for (int i = 0; i < size_; ++i) {
      const Eigen::Matrix<Type, Size, 1> 
          predictors = Eigen::Matrix<Type, Size, 1>::Random(dimension_, 1);
      const Type response = coefficients.dot(predictors) + bias 
          + scale * noise_distribution(engine);
      predictors_.col(i) = predictors;
      responses_(i) = response;
      const Type weight = weight_distribution(engine);
      weights_(i) = weight;
    }
    
  }
};

template <class Model>
class LinearModelTest : public testing::Test {
public:
  typedef typename Model::type type;
  static constexpr int size = Model::size;
protected:
  virtual void SetUp() {}
  static const std::vector<int> sizes;
  static const std::vector<typename Model::type> discounts;
  static const std::vector<typename Model::type> regularizers;
  static const typename Model::type tolerance;
};

template <class Model>
const std::vector<int> 
    LinearModelTest<Model>::sizes = 
        {10, 20, 50, 100, 200, 500, 1000};

template <class Model>
const std::vector<typename Model::type> 
    LinearModelTest<Model>::discounts = 
        {0.0, 0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1.0};

template <class Model>
const std::vector<typename Model::type> 
    LinearModelTest<Model>::regularizers = {0.001, 0.01, 0.1, 1.0, 10.0};

template <class Model>
const typename Model::type LinearModelTest<Model>::tolerance = 
    std::sqrt(std::numeric_limits<typename Model::type>::epsilon());

template <class Model>
class StaticLinearModelTest : public LinearModelTest<Model> {
  static_assert(Model::size != Eigen::Dynamic, "");
protected:
  static constexpr int dimension = Model::size;
};

typedef testing::Types<LinearModel<double, 1>, 
                       LinearModel<double, 2>, 
                       LinearModel<double, 5>, 
                       LinearModel<double, 10>> StaticTypes;

TYPED_TEST_CASE(StaticLinearModelTest, StaticTypes);

// Check that batch and incremental estimates are the same.
TYPED_TEST(StaticLinearModelTest, IsEqualToBatch) {
  const int dimension = this->dimension;
  for (const int size : this->sizes) {
    
    using Type = typename TypeParam::type;
    const int Size = TypeParam::size;
    
    // Create test sample.
    TestSample<Type, Size> sample(dimension, size);
    
    ASSERT_EQ(sample.getDimension(), dimension);
    ASSERT_EQ(sample.getSize(), size);
    
    for (const Type discount : this->discounts) {
      for (const Type regularizer : this->regularizers) {
        
        // Fit parameters.
        const std::pair<Eigen::Matrix<Type, Size, 1>, Type> 
            parameters = sample.fitParameters(discount, regularizer);
        
        const Eigen::Matrix<Type, Size, 1> coefficients = parameters.first;
        const Type bias = parameters.second;
        
        LinearModel<Type, Size> model(discount, regularizer);
        
        ASSERT_EQ(model.getDimension(), dimension);
        ASSERT_EQ(model.getDiscount(), discount);
        ASSERT_EQ(model.getRegularizer(), regularizer);
        
        // Check dimensions.
        ASSERT_EQ(model.getCoefficients().rows(), dimension);
        ASSERT_EQ(model.getCoefficients().cols(), 1);
        
        EXPECT_EQ(model.getCoefficients().array().abs().maxCoeff(), 0.0);
        EXPECT_EQ(model.getBias(), 0.0);
        
        // Fit parameters incrementally.
        for (int i = 0; i < size; ++i) {
          model.update(sample.getPredictors(i), sample.getResponse(i), 
              sample.getWeight(i));
        }
        
        // Batch and incremental estimates should match.
        EXPECT_LT((model.getCoefficients() - coefficients)
            .array().abs().maxCoeff(), this->tolerance);
        EXPECT_LT(std::abs(model.getBias() - bias), this->tolerance);
        
      }
    }
    
  }
}

template <class Model>
class DynamicLinearModelTest : public LinearModelTest<Model> {
  static_assert(Model::size == Eigen::Dynamic, "");
protected:
  static const std::vector<int> dimensions;
};

template <class Model>
const std::vector<int> 
    DynamicLinearModelTest<Model>::dimensions = {1, 2, 5, 10};

typedef testing::Types<LinearModel<double, Eigen::Dynamic>> DynamicTypes;

TYPED_TEST_CASE(DynamicLinearModelTest, DynamicTypes);

// Check that batch and incremental estimates are the same.
TYPED_TEST(DynamicLinearModelTest, IsEqualToBatch) {
  for (const int dimension : this->dimensions) {
    for (const int size : this->sizes) {
      
      using Type = typename TypeParam::type;
      const int Size = TypeParam::size;
      
      // Create test sample.
      TestSample<Type, Size> sample(dimension, size);
      
      ASSERT_EQ(sample.getDimension(), dimension);
      ASSERT_EQ(sample.getSize(), size);
      
      for (const Type discount : this->discounts) {
        for (const Type regularizer : this->regularizers) {
          
          // Fit parameters.
          const std::pair<Eigen::Matrix<Type, Size, 1>, Type> 
              parameters = sample.fitParameters(discount, regularizer);
          
          const Eigen::Matrix<Type, Size, 1> coefficients = parameters.first;
          const Type bias = parameters.second;
          
          LinearModel<Type, Size> model(dimension, discount, regularizer);
          
          ASSERT_EQ(model.getDimension(), dimension);
          ASSERT_EQ(model.getDiscount(), discount);
          ASSERT_EQ(model.getRegularizer(), regularizer);
          
          // Check dimensions.
          ASSERT_EQ(model.getCoefficients().rows(), dimension);
          ASSERT_EQ(model.getCoefficients().cols(), 1);
          
          EXPECT_EQ(model.getCoefficients().array().abs().maxCoeff(), 0.0);
          EXPECT_EQ(model.getBias(), 0.0);
          
          // Fit parameters incrementally.
          for (int i = 0; i < size; ++i) {
            model.update(sample.getPredictors(i), sample.getResponse(i), 
                sample.getWeight(i));
          }
          
          // Batch and incremental estimates should match.
          EXPECT_LT((model.getCoefficients() - coefficients)
              .array().abs().maxCoeff(), this->tolerance);
          EXPECT_LT(std::abs(model.getBias() - bias), this->tolerance);
          
        }
      }
      
    }
  }
}

} // namespace numerical_methods

int main(int num_arguments, char** arguments) {
  testing::InitGoogleTest(&num_arguments, arguments);
  return RUN_ALL_TESTS();
}
