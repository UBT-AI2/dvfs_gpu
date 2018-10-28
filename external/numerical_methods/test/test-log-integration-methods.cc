#include <cmath>
#include <functional>

#include <gtest/gtest.h>

#include "numerical-methods/common-definitions.h"
#include "numerical-methods/integration/integration-method.h"
#include "numerical-methods/integration/log-tanh-sinh-method.h"

namespace numerical_methods {

// Problem for testing integration methods.
template <typename Type>
struct TestProblem {
TestProblem(const std::function<Type(Type)>& function, Type a, Type b, 
    Type value, Type tolerance) : function(function), a(a), b(b), 
    value(value), tolerance(tolerance) {
  CHECK_LT(a, b) << "Integration interval must be non-empty.";
  CHECK_GE(tolerance, 0.0) << "Tolerance must be non-negative.";
}
typedef Type type;
const std::function<Type(Type)> function;
const Type a, b;
const Type value;
const Type tolerance;
};

// Create suite of log-integration test problems. Problems consist of 
// computing log-normalization constant of known probability density function.
template <typename Type>
std::vector<TestProblem<Type>> createTestProblems() {
  std::vector<TestProblem<Type>> problems;
  {
    
    // Log-normalization constant of uniform distribution.
    const Type alpha = 2.0, beta = 5.0;
    const std::function<Type(Type)> function = [alpha, beta](Type x) -> Type {
      return 0.0;
    };
    const Type a = alpha, b = beta;
    const Type value = std::log(beta - alpha);
    const Type tolerance = 0.0;
    TestProblem<Type> problem(function, a, b, value, tolerance);
    problems.push_back(problem);
    
  }
  {
    
    // Log-normalization constant of beta distribution.
    const Type alpha = 5.0, beta = 5.0;
    const std::function<Type(Type)> function = [alpha, beta](Type x) -> Type {
      x = std::min<Type>(x, 1.0 - getEps<Type>());
      x = std::max<Type>(x, getEps<Type>());
      return (alpha - 1.0) * std::log(x) + (beta - 1.0) * std::log(1.0 - x);
    };
    const Type a = 0.0, b = 1.0;
    const Type value = std::lgamma(alpha) + std::lgamma(beta) 
        - std::lgamma(alpha + beta);
    const Type tolerance = 0.0;
    TestProblem<Type> problem(function, a, b, value, tolerance);
    problems.push_back(problem);
    
  }
  {
    
    // Log-normalization constant of logit normal distribution.
    const Type mu = 0.0, sigma = 2.0;
    const std::function<Type(Type)> function = [mu, sigma](Type x) -> Type {
      x = std::min<Type>(x, 1.0 - getEps<Type>());
      x = std::max<Type>(x, getEps<Type>());
      return - std::log(x * (1.0 - x)) 
          - std::pow((std::log(x / (1.0 - x)) - mu) / sigma, 2) / 2.0;
    };
    const Type a = 0.0, b = 1.0;
    const Type value = std::log(2.0 * getPi<Type>()) / 2.0 + std::log(sigma);
    const Type tolerance = 0.0;
    TestProblem<Type> problem(function, a, b, value, tolerance);
    problems.push_back(problem);
    
  }
  {
    
    // Log-normalization constant of semicircle distribution.
    const Type rho = 2.0;
    const std::function<Type(Type)> function = [rho](Type x) -> Type {
      x = std::min<Type>(x, rho - getEps<Type>());
      x = std::max<Type>(x, - rho + getEps<Type>());
      return std::log(std::pow(rho, 2) - std::pow(x, 2)) / 2.0;
    };
    const Type a = - rho, b = rho;
    const Type value = std::log(getPi<Type>() / 2.0) + 2.0 * std::log(rho);
    const Type tolerance = 0.0;
    TestProblem<Type> problem(function, a, b, value, tolerance);
    problems.push_back(problem);
    
  }
  {
    
    // Log-normalization constant of raised cosine distribution.
    const Type mu = 0.0, sigma = 3.0;
    const std::function<Type(Type)> function = [mu, sigma](Type x) -> Type {
      x = std::min<Type>(x, mu + sigma / 2.0 - getEps<Type>());
      x = std::max<Type>(x, mu - sigma / 2.0 + getEps<Type>());
      return std::log1p(std::cos(2.0 * getPi<Type>() * (x - mu) / sigma));
    };
    const Type a = mu - sigma / 2.0, b = mu + sigma / 2.0;
    const Type value = std::log(sigma);
    const Type tolerance = 0.0;
    TestProblem<Type> problem(function, a, b, value, tolerance);
    problems.push_back(problem);
    
  }
  {
    
    // Log-normalization constant of Kumaraswamy distribution.
    const Type alpha = 5.0, beta = 1.0;
    const std::function<Type(Type)> function = [alpha, beta](Type x) -> Type {
      x = std::min<Type>(x, 1.0 - getEps<Type>());
      x = std::max<Type>(x, getEps<Type>());
      return (alpha - 1.0) * std::log(x) 
          + (beta - 1.0) * std::log(1.0 - std::pow(x, alpha));
    };
    const Type a = 0.0, b = 1.0;
    const Type value = - std::log(alpha) - std::log(beta);
    const Type tolerance = 0.0;
    TestProblem<Type> problem(function, a, b, value, tolerance);
    problems.push_back(problem);
    
  }
  return problems;
}

template <class Method>
class LogIntegrationMethodTest : public testing::Test {
public:
  typedef typename Method::type type;
protected:
  virtual void SetUp() {}
  static const std::vector<TestProblem<typename Method::type>> problems;
  static const std::vector<typename Method::type> errors;
};

template <class Method>
const std::vector<TestProblem<typename Method::type>> 
    LogIntegrationMethodTest<Method>::problems = 
        createTestProblems<typename Method::type>();

template <class Method>
const std::vector<typename Method::type> 
    LogIntegrationMethodTest<Method>::errors = 
        {1.0e-3, 1.0e-6, 1.0e-9, 1.0e-12};

typedef testing::Types<LogTanhSinhMethod<double>> Types;

TYPED_TEST_CASE(LogIntegrationMethodTest, Types);

// Check that integration method achieves desired error.
TYPED_TEST(LogIntegrationMethodTest, AchievesDesiredError) {
  using Type = typename TypeParam::type;
  for (Type error : this->errors) {
    TypeParam method(error);
    for (const TestProblem<Type>& problem : this->problems) {
      Type value = method.integrate(problem.function, problem.a, problem.b);
      EXPECT_LT(std::abs(value - problem.value), error + problem.tolerance);
    }
  }
}

} // namespace numerical_methods

int main(int num_arguments, char** arguments) {
  testing::InitGoogleTest(&num_arguments, arguments);
  return RUN_ALL_TESTS();
}