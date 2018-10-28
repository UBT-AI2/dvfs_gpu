#include <cmath>
#include <functional>

#include <gtest/gtest.h>

#include "numerical-methods/common-definitions.h"
#include "numerical-methods/integration/integration-method.h"
#include "numerical-methods/integration/tanh-sinh-method.h"

namespace numerical_methods {

// Problem for testing integration methods.
template <typename Type>
struct TestProblem {
TestProblem(const std::function<Type(Type)>& function, Type a, Type b, 
    Type value, Type margin) : function(function), a(a), b(b), 
    value(value), margin(margin) {
  CHECK_LT(a, b) << "Integration interval must be non-empty.";
  CHECK_GE(margin, 0.0) << "Margin must be non-negative.";
}
typedef Type type;
const std::function<Type(Type)> function;
const Type a, b;
const Type value;
const Type margin;
};

// Create suite of integration test problems, with five categories:
//   a. Continuous, well-behaved functions (all derivatives exist and are 
//      bounded), defined on finite intervals;
//   b. Continuous functions defined on finite intervals, with an infinite 
//      derivative at an endpoint;
//   c. Continuous Functions defined on finite intervals, with an integrable 
//      singularity at an endpoint;
//   d. Functions defined on an infinite interval, warped to a finite interval; 
//      and
//   e. Oscillatory defined functions on an infinite interval, warped to a 
//      finite interval.
// Based on Bailey et al. (2005).
// 
// D. H. Bailey, K. Jeyabalan and X. S. Li, "A Comparison of Three High-
// precision Quadrature Schemes," in Experimental Mathematics, vol. 14, no. 4, 
// pp. 317-329 (2005).
template <typename Type>
std::vector<TestProblem<Type>> createTestProblems() {
  std::vector<TestProblem<Type>> problems;
  {
    
    // Test problem 1 (category a.) of Bailey et al. (2005).
    const std::function<Type(Type)> function = [](Type x) -> Type {
      return x * std::log1p(x);
    };
    const Type a = 0.0, b = 1.0;
    const Type value = 1.0 / 4.0;
    const Type margin = 0.0;
    TestProblem<Type> problem(function, a, b, value, margin);
    problems.push_back(problem);
    
  }
  {
    
    // Test problem 2 (category a.) of Bailey et al. (2005).
    const std::function<Type(Type)> function = [](Type x) -> Type {
      x = std::min<Type>(x, 1.0 - getEps<Type>());
      return std::pow(x, 2) * std::atan(x);
    };
    const Type a = 0.0, b = 1.0;
    const Type value = (getPi<Type>() - 2.0 
        + 2.0 * std::log(2.0)) / 12.0;
    const Type margin = 0.0;
    TestProblem<Type> problem(function, a, b, value, margin);
    problems.push_back(problem);
    
  }
  {
    
    // Test problem 3 (category a.) of Bailey et al. (2005).
    const std::function<Type(Type)> function = [](Type x) -> Type {
      return std::exp(x) * std::cos(x);
    };
    const Type a = 0.0, b = getPi<Type>() / 2.0;
    const Type value = (std::exp(getPi<Type>() / 2.0) - 1.0) / 2.0;
    const Type margin = 0.0;
    TestProblem<Type> problem(function, a, b, value, margin);
    problems.push_back(problem);
    
  }
  {
    
    // Test problem 4 (category a.) of Bailey et al. (2005).
    const std::function<Type(Type)> function = [](Type x) -> Type {
      const Type a = std::pow(x, 2);
      const Type b = std::sqrt(2.0 + a);
      return std::atan(b) / ((1.0 + a) * b);
    };
    const Type a = 0.0, b = 1;
    const Type value = 5.0 * std::pow(getPi<Type>(), 2) / 96.0;
    const Type margin = 0.0;
    TestProblem<Type> problem(function, a, b, value, margin);
    problems.push_back(problem);
    
  }
  {
    
    // Test problem 5 (category b.) of Bailey et al. (2005).
    const std::function<Type(Type)> function = [](Type x) -> Type {
      x = std::max<Type>(x, getEps<Type>());
      return std::sqrt(x) * std::log(x);
    };
    const Type a = 0.0, b = 1;
    const Type value = - 4.0 / 9.0;
    const Type margin = 0.0;
    TestProblem<Type> problem(function, a, b, value, margin);
    problems.push_back(problem);
    
  }
  {
    
    // Test problem 6 (category b.) of Bailey et al. (2005).
    const std::function<Type(Type)> function = [](Type x) -> Type {
      return std::sqrt(1.0 - std::pow(x, 2));
    };
    const Type a = 0.0, b = 1;
    const Type value = getPi<Type>() / 4.0;
    const Type margin = 0.0;
    TestProblem<Type> problem(function, a, b, value, margin);
    problems.push_back(problem);
    
  }
  {
    
    // Test problem 7 (category c.) of Bailey et al. (2005).
    const std::function<Type(Type)> function = [](Type x) -> Type {
      x = std::min<Type>(x, 1.0 - getEps<Type>());
      return std::sqrt(x / (1.0 - std::pow(x, 2)));
    };
    const Type a = 0.0, b = 1;
    const Type value = 2.0 * std::sqrt(getPi<Type>()) * 
        std::tgamma(3.0 / 4.0) / std::tgamma(1.0 / 4.0);
    const Type margin = 1.0e-3;
    TestProblem<Type> problem(function, a, b, value, margin);
    problems.push_back(problem);
    
  }
  {
    
    // Test problem 8 (category c.) of Bailey et al. (2005).
    const std::function<Type(Type)> function = [](Type x) -> Type {
      x = std::max<Type>(x, getEps<Type>());
      return std::pow(std::log(x), 2);
    };
    const Type a = 0.0, b = 1;
    const Type value = 2.0;
    const Type margin = 0.0;
    TestProblem<Type> problem(function, a, b, value, margin);
    problems.push_back(problem);
    
  }
  {
    
    // Test problem 9 (category c.) of Bailey et al. (2005).
    const std::function<Type(Type)> function = [](Type x) -> Type {
      x = std::min<Type>(x, getPi<Type>() / 2.0 - getEps<Type>());
      return std::log(std::cos(x));
    };
    const Type a = 0.0, b = getPi<Type>() / 2.0;
    const Type value = - getPi<Type>() * std::log(2.0) / 2.0;
    const Type margin = 0.0;
    TestProblem<Type> problem(function, a, b, value, margin);
    problems.push_back(problem);
    
  }
  {
    
    // Test problem 10 (category c.) of Bailey et al. (2005).
    const std::function<Type(Type)> function = [](Type x) -> Type {
      x = std::min<Type>(x, getPi<Type>() / 2.0 - getEps<Type>());
      return std::sqrt(std::tan(x));
    };
    const Type a = 0.0, b = getPi<Type>() / 2.0;
    const Type value = getPi<Type>() * std::sqrt(2.0) / 2.0;
    const Type margin = 1.0e-3;
    TestProblem<Type> problem(function, a, b, value, margin);
    problems.push_back(problem);
    
  }
  {
    
    // Test problem 11 (category d.) of Bailey et al. (2005).
    const std::function<Type(Type)> function = [](Type x) -> Type {
      return 1.0 / (2.0 * x * (x - 1.0) + 1.0);
    };
    const Type a = 0.0, b = 1.0;
    const Type value = getPi<Type>() / 2.0;
    const Type margin = 0.0;
    TestProblem<Type> problem(function, a, b, value, margin);
    problems.push_back(problem);
    
  }
  {
    
    // Test problem 12 (category d.) of Bailey et al. (2005).
    const std::function<Type(Type)> function = [](Type x) -> Type {
      return std::exp(1.0 - 1.0 / x) / (x * std::sqrt(x * (1.0 - x)));
    };
    const Type a = 0.0, b = 1.0;
    const Type value = std::sqrt(getPi<Type>());
    const Type margin = 1.0e-3;
    TestProblem<Type> problem(function, a, b, value, margin);
    problems.push_back(problem);
    
  }
  {
    
    // Test problem 13 (category d.) of Bailey et al. (2005).
    const std::function<Type(Type)> function = [](Type x) -> Type {
      return std::exp(- std::pow(1.0 / x - 1.0, 2) / 2.0) / std::pow(x, 2);
    };
    const Type a = 0.0, b = 1.0;
    const Type value = std::sqrt(getPi<Type>() / 2.0);
    const Type margin = 0.0;
    TestProblem<Type> problem(function, a, b, value, margin);
    problems.push_back(problem);
    
  }
  {
    
    // Test problem 14 (category e.) of Bailey et al. (2005).
    const std::function<Type(Type)> function = [](Type x) -> Type {
      return std::exp(1.0 - 1.0 / x) * std::cos(1.0 / x - 1.0) 
          / std::pow(x, 2);
    };
    const Type a = 0.0, b = 1.0;
    const Type value = 1.0 / 2.0;
    const Type margin = 0.0;
    TestProblem<Type> problem(function, a, b, value, margin);
    problems.push_back(problem);
    
  }
  return problems;
}

template <class Method>
class IntegrationMethodTest : public testing::Test {
public:
  typedef typename Method::type type;
protected:
  virtual void SetUp() {}
  static const std::vector<TestProblem<typename Method::type>> problems;
  static const std::vector<typename Method::type> tolerances;
};

template <class Method>
const std::vector<TestProblem<typename Method::type>> 
    IntegrationMethodTest<Method>::problems = 
        createTestProblems<typename Method::type>();

template <class Method>
const std::vector<typename Method::type> 
    IntegrationMethodTest<Method>::tolerances = 
        {1.0e-3, 1.0e-6, 1.0e-9, 1.0e-12};

typedef testing::Types<TanhSinhMethod<double>> Types;

TYPED_TEST_CASE(IntegrationMethodTest, Types);

// Check that integration method achieves desired tolerance.
TYPED_TEST(IntegrationMethodTest, AchievesDesiredTolerance) {
  using Type = typename TypeParam::type;
  for (Type tolerance : this->tolerances) {
    TypeParam method(tolerance);
    for (const TestProblem<Type>& problem : this->problems) {
      Type value = method.integrate(problem.function, problem.a, problem.b);
      EXPECT_LT(std::abs(value - problem.value), tolerance + problem.margin);
    }
  }
}

} // namespace numerical_methods

int main(int num_arguments, char** arguments) {
  testing::InitGoogleTest(&num_arguments, arguments);
  return RUN_ALL_TESTS();
}
