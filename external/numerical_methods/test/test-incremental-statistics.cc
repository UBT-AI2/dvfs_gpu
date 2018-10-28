#include <cmath>
#include <limits>
#include <random>
#include <utility>
#include <vector>

#include <Eigen/Dense>
#include <gtest/gtest.h>

#include "numerical-methods/common-definitions.h"
#include "numerical-methods/utility/incremental-statistics.h"

namespace numerical_methods {

// Sequence for testing incremental statistics.
template <typename Type, int Size>
class TestSequence {
public:
  explicit TestSequence(int dimension, int length) : dimension_(dimension), 
      length_(length) {
    if (Size != Eigen::Dynamic) {
      CHECK_EQ(dimension, Size) << "Dimension must be consistent.";
    } else {
      CHECK_GT(dimension, 0) << "Dimension must be positive.";
    }
    CHECK_GT(length, 0) << "Length must be positive.";
    generate();
  }
  inline int getDimension() const {
    return dimension_;
  }
  inline int getLength() const {
    return length_;
  }
  inline Type getWeight(std::size_t i) const {
    CHECK_LT(i, length_);
    return weights_(i);
  }
  inline Eigen::Matrix<Type, Size, 1> getPoint(std::size_t i) const {
    CHECK_LT(i, length_);
    return values_.col(i);
  }
  std::pair<Eigen::Matrix<Type, Size, 1>, Eigen::Matrix<Type, Size, Size>> 
      evalStatistics(Type discount) {
    
    CHECK_GE(discount, 0.0) << "Discount must be non-negative.";
    CHECK_LE(discount, 1.0) << "Discount must be less than or equal to one.";
    
    Eigen::Matrix<Type, Size, 1> mean(dimension_);
    Eigen::Matrix<Type, Size, Size> covariance(dimension_, dimension_);
    
    // Compute mean.
    mean.setZero();
    Type sum = 0.0;
    for (int i = 0; i < length_; ++i) {
      const Type 
          weight = weights_(i) * std::pow(1.0 - discount, length_ - 1 - i);
      mean += weight * values_.col(i);
      sum += weight;
    }
    mean /= sum;
    
    // Compute covariance.
    covariance.setZero();
    for (int i = 0; i < length_; ++i) {
      const Type 
          weight = weights_(i) * std::pow(1.0 - discount, length_ - 1 - i);
      const Eigen::Matrix<Type, Size, 1> residual = values_.col(i) - mean;
      covariance += weight * residual * residual.transpose();
    }
    covariance /= sum;
    
    return std::make_pair(mean, covariance);
    
  }
private:
  const int dimension_;
  const int length_;
  Eigen::Matrix<Type, Size, Eigen::Dynamic> values_;
  Eigen::Matrix<Type, Eigen::Dynamic, 1> weights_;
  void generate() {
    
    std::uniform_real_distribution<Type> scale_distribution(1.0, 5.0);
    std::uniform_real_distribution<Type> skewness_distribution(- 2.0, 2.0);
    std::uniform_real_distribution<Type> weight_distribution(0.0, 1.0);
    std::normal_distribution<Type> noise_distribution(0.0, 1.0);
    
    std::mt19937 engine;
    std::random_device device;
    engine.seed(device());
    
    // Generate random scale and skewness parameters.
    const Type scale = scale_distribution(engine);
    const Type skewness = skewness_distribution(engine);
    
    values_.resize(dimension_, length_);
    weights_.resize(length_);
    
    // Generate sequence from stochastic process.
    Eigen::Matrix<Type, Size, 1> point(dimension_);
    point.setZero();
    for (int i = 0; i < length_; ++i) {
      for (int j = 0; j < dimension_; ++j) {
        Type noise = scale * noise_distribution(engine);
        if (scale * noise_distribution(engine) > skewness * noise) {
          noise *= - 1.0;
        }
        point(j) += noise;
      }
      values_.col(i) = point;
      const Type weight = weight_distribution(engine);
      weights_(i) = weight;
    }
    
  }
};

template <class Statistics>
class IncrementalStatisticsTest : public testing::Test {
public:
  typedef typename Statistics::type type;
  static constexpr int size = Statistics::size;
protected:
  virtual void SetUp() {}
  static const std::vector<int> lengths;
  static const std::vector<typename Statistics::type> discounts;
  static const typename Statistics::type tolerance;
};

template <class Statistics>
const std::vector<int> 
    IncrementalStatisticsTest<Statistics>::lengths = 
        {1, 2, 5, 10, 20, 50, 100, 200, 500, 1000};

template <class Statistics>
const std::vector<typename Statistics::type> 
    IncrementalStatisticsTest<Statistics>::discounts = 
        {0.0, 0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1.0};

template <class Statistics>
const typename Statistics::type 
    IncrementalStatisticsTest<Statistics>::tolerance = 
        std::sqrt(std::numeric_limits<typename Statistics::type>::epsilon());

template <class Statistics>
class StaticIncrementalStatisticsTest : 
    public IncrementalStatisticsTest<Statistics> {
  static_assert(Statistics::size != Eigen::Dynamic, "");
protected:
  static constexpr int dimension = Statistics::size;
};

typedef testing::Types<IncrementalStatistics<double, 1>, 
                       IncrementalStatistics<double, 2>, 
                       IncrementalStatistics<double, 5>, 
                       IncrementalStatistics<double, 10>> StaticTypes;

TYPED_TEST_CASE(StaticIncrementalStatisticsTest, StaticTypes);

// Check that batch and incremental statistics are the same.
TYPED_TEST(StaticIncrementalStatisticsTest, IsEqualToBatch) {
  const int dimension = this->dimension;
  for (int length : this->lengths) {
    
    using Type = typename TypeParam::type;
    const int Size = TypeParam::size;
    
    // Create test sequence.
    TestSequence<Type, Size> sequence(dimension, length);
    
    ASSERT_EQ(sequence.getDimension(), dimension);
    ASSERT_EQ(sequence.getLength(), length);
    
    for (Type discount : this->discounts) {
      
      // Compute batch statistics.
      const std::pair<Eigen::Matrix<Type, Size, 1>, 
          Eigen::Matrix<Type, Size, Size>> 
              summary = sequence.evalStatistics(discount);
      
      const Eigen::Matrix<Type, Size, 1> mean = summary.first;
      const Eigen::Matrix<Type, Size, Size> covariance = summary.second;
      
      IncrementalStatistics<Type, Size> statistics(discount);
      
      ASSERT_EQ(statistics.getDimension(), dimension);
      ASSERT_EQ(statistics.getDiscount(), discount);
      
      // Check dimensions.
      ASSERT_EQ(statistics.getMean().rows(), dimension);
      ASSERT_EQ(statistics.getMean().cols(), 1);
      ASSERT_EQ(statistics.getCovariance().rows(), dimension);
      ASSERT_EQ(statistics.getCovariance().cols(), dimension);
      
      EXPECT_EQ(statistics.getMean().array().abs().maxCoeff(), 0.0);
      EXPECT_EQ(statistics.getCovariance().array().abs().maxCoeff(), 0.0);
      
      // Compute incremental statistics.
      for (int i = 0; i < length; ++i) {
        statistics.update(sequence.getPoint(i), sequence.getWeight(i));
      }
      
      // Batch and incremental statistics should match.
      EXPECT_LT((statistics.getMean() - mean)
          .array().abs().maxCoeff(), this->tolerance);
      EXPECT_LT((statistics.getCovariance() - covariance)
          .array().abs().maxCoeff(), this->tolerance);
      
    }
    
  }
}

template <class Statistics>
class DynamicIncrementalStatisticsTest : 
    public IncrementalStatisticsTest<Statistics> {
  static_assert(Statistics::size == Eigen::Dynamic, "");
protected:
  static const std::vector<int> dimensions;
};

template <class Statistics>
const std::vector<int> 
    DynamicIncrementalStatisticsTest<Statistics>::dimensions = {1, 2, 5, 10};

typedef testing::Types<IncrementalStatistics<double, Eigen::Dynamic>> 
    DynamicTypes;

TYPED_TEST_CASE(DynamicIncrementalStatisticsTest, DynamicTypes);

// Check that batch and incremental statistics are the same.
TYPED_TEST(DynamicIncrementalStatisticsTest, IsEqualToBatch) {
  for (int dimension : this->dimensions) {
    for (int length : this->lengths) {
      
      using Type = typename TypeParam::type;
      const int Size = TypeParam::size;
      
      // Create test sequence.
      TestSequence<Type, Size> sequence(dimension, length);
      
      ASSERT_EQ(sequence.getDimension(), dimension);
      ASSERT_EQ(sequence.getLength(), length);
      
      for (Type discount : this->discounts) {
        
        // Compute batch statistics.
        const std::pair<Eigen::Matrix<Type, Size, 1>, 
            Eigen::Matrix<Type, Size, Size>> 
                summary = sequence.evalStatistics(discount);
        
        const Eigen::Matrix<Type, Size, 1> mean = summary.first;
        const Eigen::Matrix<Type, Size, Size> covariance = summary.second;
        
        IncrementalStatistics<Type, Size> statistics(dimension, discount);
        
        ASSERT_EQ(statistics.getDimension(), dimension);
        ASSERT_EQ(statistics.getDiscount(), discount);
        
        // Check dimensions.
        ASSERT_EQ(statistics.getMean().rows(), dimension);
        ASSERT_EQ(statistics.getMean().cols(), 1);
        ASSERT_EQ(statistics.getCovariance().rows(), dimension);
        ASSERT_EQ(statistics.getCovariance().cols(), dimension);
        
        EXPECT_EQ(statistics.getMean().array().abs().maxCoeff(), 0.0);
        EXPECT_EQ(statistics.getCovariance().array().abs().maxCoeff(), 0.0);
        
        // Compute incremental statistics.
        for (int i = 0; i < length; ++i) {
          statistics.update(sequence.getPoint(i), sequence.getWeight(i));
        }
        
        // Batch and incremental statistics should match.
        EXPECT_LT((statistics.getMean() - mean)
            .array().abs().maxCoeff(), this->tolerance);
        EXPECT_LT((statistics.getCovariance() - covariance)
            .array().abs().maxCoeff(), this->tolerance);
        
      }
      
    }
  }
}

} // namespace numerical_methods

int main(int num_arguments, char** arguments) {
  testing::InitGoogleTest(&num_arguments, arguments);
  return RUN_ALL_TESTS();
}
