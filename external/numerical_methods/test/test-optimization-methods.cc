#include <cmath>
#include <functional>
#include <initializer_list>

#include <Eigen/Dense>
#include <gtest/gtest.h>

#include "numerical-methods/common-definitions.h"
#include "numerical-methods/optimization/optimization-method.h"
#include "numerical-methods/optimization/nelder-mead-method.h"

namespace numerical_methods {

// Suite of test problems for optimization methods.
template <typename Type, int Size>
class TestSuite {
public:
  
  TestSuite(int dimension, Type tolerance) : dimension_(dimension), 
      tolerance_(tolerance) {
    if (Size != Eigen::Dynamic) {
      CHECK_EQ(dimension, Size) << "Dimension must be consistent.";
    } else {
      CHECK_GT(dimension, 0) << "Dimension must be positive.";
    }
    CHECK_GE(tolerance, 0.0) << "Tolerance must be non-negative.";
    initialize();
  }
  
  inline int getDimension() const {
    return dimension_;
  }
  inline Type getTolerance() const {
    return tolerance_;
  }
  
  class TestProblem {
  public:
    TestProblem(int dimension, 
        const std::function<Type(const Eigen::Matrix<Type, Size, 1>&)>& 
            function, 
        const Eigen::Matrix<Type, Size, 1>& init_guess, 
        const Eigen::Matrix<Type, Size, 1>& glob_minimum, 
        Type param_tolerance) : dimension_(dimension), 
                                function_(function), 
                                init_guess_(init_guess), 
                                glob_minimum_(glob_minimum), 
                                param_tolerance_(param_tolerance) {
      if (Size != Eigen::Dynamic) {
        CHECK_EQ(dimension, Size) << "Dimension must be consistent.";
      } else {
        CHECK_GT(dimension, 0) << "Dimension must be positive.";
      }
    }
    int getDimension() const {
      return dimension_;
    }
    const std::function<Type(const Eigen::Matrix<Type, Size, 1>&)>& 
        getFunction() const {
      return function_;
    }
    const Eigen::Matrix<Type, Size, 1>& getInitGuess() const {
      return init_guess_;
    }
    const Eigen::Matrix<Type, Size, 1>& getGlobMinimum() const {
      return glob_minimum_;
    }
    Type getParamTolerance() const {
      return param_tolerance_;
    }
    typedef Type type;
  private:
    const int dimension_;
    const std::function<Type(const Eigen::Matrix<Type, Size, 1>&)> function_;
    const Eigen::Matrix<Type, Size, 1> init_guess_;
    const Eigen::Matrix<Type, Size, 1> glob_minimum_;
    const Type param_tolerance_;
  };
  
  std::size_t getNumProblems() const {
    return problems_.size();
  }
  const TestProblem& getProblem(std::size_t i) const {
    CHECK_LT(i, problems_.size());
    return problems_[i];
  }
  
private:
  
  const int dimension_;
  const Type tolerance_;
  
  std::vector<TestProblem> problems_;
  
  // Create suite of function minimization test problems. Based on Surjanovic 
  // and Bingham (2013).
  // 
  // S. Surjanovic and D. Bingham, "Virtual Library of Simulation Experiments 
  // - Test Functions and Datasets: Optimization Test Problems," available 
  // online at http://www.sfu.ca/~ssurjano/optimization.html [accessed on 
  // April, 2015], Simon Fraser University (2013).
  void initialize() {
    
    std::function<Type(const Eigen::Matrix<Type, Size, 1>&)> function;
    Eigen::Matrix<Type, Size, 1> init_guess(dimension_);
    Eigen::Matrix<Type, Size, 1> glob_minimum(dimension_);
      
    // Rosenbrock function in S. Surjanovic and D. Bingham (2013).
    function = [this](const Eigen::Matrix<Type, Size, 1>& x) -> Type {
      Type value = 0.0;
      for (int i = 1; i < this->dimension_; ++i) {
        value += 100.0 * std::pow(x(i) - std::pow(x(i - 1), 2), 2) 
            + std::pow(x(i - 1) - 1.0, 2);
      }
      return value;
    };
    init_guess.fill(- 1.0);
    glob_minimum.setOnes();
    problems_.push_back(TestProblem(dimension_, function, init_guess, 
        glob_minimum, tolerance_));
    
    // Rotated ellipsoid function in S. Surjanovic and D. Bingham (2013).
    function = [this](const Eigen::Matrix<Type, Size, 1>& x) -> Type {
      Type value = 0.0;
      for (int i = 0; i < this->dimension_; ++i) {
        for (int j = 0; j <= i; ++j) {
          value += std::pow(x(j), 2);
        }
      }
      return value;
    };
    for (int i = 0; i < dimension_; ++i) {
      init_guess(i) = std::pow(- 1.0, i);
    }
    glob_minimum.setZero();
    problems_.push_back(TestProblem(dimension_, function, init_guess, 
        glob_minimum, tolerance_));
    
    // Sum-of-powers function in S. Surjanovic and D. Bingham (2013).
    function = [this](const Eigen::Matrix<Type, Size, 1>& x) -> Type {
      Type value = 0.0;
      for (int i = 0; i < this->dimension_; ++i) {
        value += std::pow(std::abs(x(i)), i + 2);
      }
      return value;
    };
    init_guess.setOnes();
    glob_minimum.setZero();
    problems_.push_back(TestProblem(dimension_, function, init_guess, 
        glob_minimum, tolerance_));
    
    // Zakharov function in S. Surjanovic and D. Bingham (2013).
    function = [this](const Eigen::Matrix<Type, Size, 1>& x) -> Type {
      Type value = 0.0;
      Type increment = 0.0;
      for (int i = 0; i < this->dimension_; ++i) {
        value += static_cast<Type>(i + 1) * x(i) / 2.0;
        increment += std::pow(x(i), 2);
      }
      value = std::pow(value, 2);
      value *= value + 1.0;
      value += increment;
      return value;
    };
    for (int i = 0; i < dimension_; ++i) {
      init_guess(i) = std::pow(- 1.0, i + 1);
    }
    glob_minimum.setZero();
    problems_.push_back(TestProblem(dimension_, function, init_guess, 
        glob_minimum, tolerance_));
    
    // Dixon-Price function in S. Surjanovic and D. Bingham (2013).
    function = [this](const Eigen::Matrix<Type, Size, 1>& x) -> Type {
      Type value = std::pow(x(0) - 1.0, 2);
      for (int i = 1; i < this->dimension_; ++i) {
        value += static_cast<Type>(i + 1) * std::pow(2.0 * std::pow(x(i), 2) 
            - x(i - 1), 2);
      }
      return value;
    };
    init_guess.setOnes();
    for (int i = 0; i < dimension_; ++i) {
      glob_minimum(i) = std::pow(2.0, 1.0 / std::pow(2.0, i) - 1.0);
    }
    problems_.push_back(TestProblem(dimension_, function, init_guess, 
        glob_minimum, tolerance_));
    
    // Styblinski-Tang function in S. Surjanovic and D. Bingham (2013).
    function = [this](const Eigen::Matrix<Type, Size, 1>& x) -> Type {
      Type value = 0.0;
      for (int i = 0; i < this->dimension_; ++i) {
        value += (std::pow(x(i), 4) - 16.0 * std::pow(x(i), 2) 
            + 5.0 * x(i)) / 2.0;
      }
      return value;
    };
    init_guess.fill(- 1.0);
    glob_minimum.fill(- 2.9035);
    problems_.push_back(TestProblem(dimension_, function, init_guess, 
        glob_minimum, tolerance_));
    
  }
  
};

template <class Method>
class OptimizationMethodTest : public testing::Test {
public:
  typedef typename Method::type type;
  static constexpr int size = Method::size;
protected:
  virtual void SetUp() {}
  static const std::vector<int> dimensions;
  static constexpr typename Method::type tolerance = 1.0e-2;
  static constexpr typename Method::type min_iterations = 10;
  static constexpr typename Method::type max_iterations = 2000;
  static constexpr typename Method::type param_tolerance = 1.0e-9;
  static constexpr typename Method::type func_tolerance = 1.0e-6;
};

template <class Method>
const std::vector<int> 
    OptimizationMethodTest<Method>::dimensions = {2, 3, 4, 5};

template <class Method>
constexpr typename Method::type OptimizationMethodTest<Method>::tolerance;
template <class Method>
constexpr typename Method::type OptimizationMethodTest<Method>::min_iterations;
template <class Method>
constexpr typename Method::type OptimizationMethodTest<Method>::max_iterations;
template <class Method>
constexpr typename Method::type 
    OptimizationMethodTest<Method>::param_tolerance;
template <class Method>
constexpr typename Method::type 
    OptimizationMethodTest<Method>::func_tolerance;

template <class Method>
class StaticOptimizationMethodTest : public OptimizationMethodTest<Method> {
  static_assert(Method::size != Eigen::Dynamic, "");
protected:
  static constexpr int dimension = Method::size;
};

typedef testing::Types<NelderMeadMethod<double, 2>, 
                       NelderMeadMethod<double, 3>, 
                       NelderMeadMethod<double, 4>, 
                       NelderMeadMethod<double, 5>> StaticTypes;
                       
TYPED_TEST_CASE(StaticOptimizationMethodTest, StaticTypes);

// Check that integration method achieves desired error.
TYPED_TEST(StaticOptimizationMethodTest, FindsMinimum) {
  
  using Type = typename TypeParam::type;
  const int Size = TypeParam::size;
  
  const int dimension = this->dimension;
  const Type tolerance = this->tolerance;
  
  // Create test suite.
  TestSuite<Type, Size> suite(dimension, tolerance);
  
  ASSERT_EQ(suite.getDimension(), dimension);
  ASSERT_EQ(suite.getTolerance(), tolerance);
  
  for (std::size_t i = 0; i < suite.getNumProblems(); ++i) {
    
    const typename TestSuite<Type, Size>::TestProblem& 
        problem = suite.getProblem(i);
    
    TypeParam method;
    
    // Set optimization options.
    method.options.setMinIterations(this->min_iterations);
    method.options.setMaxIterations(this->max_iterations);
    method.options.setParamTolerance(this->param_tolerance);
    method.options.setFuncTolerance(this->func_tolerance);
    
    // Solve problem.
    const Eigen::Matrix<Type, Size, 1> 
        glob_minimum = method.minimize(problem.getFunction(), 
                                       problem.getInitGuess());
    
    // Minimum should be close to solution.
    EXPECT_LT((glob_minimum - problem.getGlobMinimum())
        .array().abs().maxCoeff(), problem.getParamTolerance());
    
  }
  
}

template <class Method>
class DynamicOptimizationMethodTest : public OptimizationMethodTest<Method> {
  static_assert(Method::size == Eigen::Dynamic, "");
protected:
  static const std::vector<int> dimensions;
};

template <class Method>
const std::vector<int> 
    DynamicOptimizationMethodTest<Method>::dimensions = {2, 3, 4, 5};

typedef testing::Types<NelderMeadMethod<double, Eigen::Dynamic>> DynamicTypes;

TYPED_TEST_CASE(DynamicOptimizationMethodTest, DynamicTypes);

// Check that integration method achieves desired error.
TYPED_TEST(DynamicOptimizationMethodTest, FindsMinimum) {
  for (int dimension : this->dimensions) {
    
    using Type = typename TypeParam::type;
    const int Size = TypeParam::size;
    
    const Type tolerance = this->tolerance;
    
    // Create test suite.
    TestSuite<Type, Size> suite(dimension, tolerance);
    
    ASSERT_EQ(suite.getDimension(), dimension);
    ASSERT_EQ(suite.getTolerance(), tolerance);
    
    for (std::size_t i = 0; i < suite.getNumProblems(); ++i) {
      
      const typename TestSuite<Type, Size>::TestProblem& 
          problem = suite.getProblem(i);
      
      TypeParam method(problem.getDimension());
      
      // Set optimization options.
      method.options.setMinIterations(this->min_iterations);
      method.options.setMaxIterations(this->max_iterations);
      method.options.setParamTolerance(this->param_tolerance);
      method.options.setFuncTolerance(this->func_tolerance);
      
      // Solve problem.
      const Eigen::Matrix<Type, Size, 1> 
          glob_minimum = method.minimize(problem.getFunction(), 
                                         problem.getInitGuess());
      
      EXPECT_LT((glob_minimum - problem.getGlobMinimum())
          .array().abs().maxCoeff(), problem.getParamTolerance());
      
    }
    
  }
}

} // namespace numerical_methods

int main(int num_arguments, char** arguments) {
  testing::InitGoogleTest(&num_arguments, arguments);
  return RUN_ALL_TESTS();
}
