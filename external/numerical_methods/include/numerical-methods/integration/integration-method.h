#ifndef NUMERICAL_METHODS_INTEGRATION_INTEGRATION_METHOD_H_
#define NUMERICAL_METHODS_INTEGRATION_INTEGRATION_METHOD_H_

#include "numerical-methods/common-definitions.h"

namespace numerical_methods {

// This class is a base class for all integration methods. An integration 
// method computes the integral of a real-valued function on a given interval, 
// within a specified tolerance.
template <typename Type>
class IntegrationMethod {
public:
  
  typedef Type type;
  
  explicit IntegrationMethod(Type tolerance) : tolerance_(tolerance) {
    CHECK_GT(tolerance, Type(0.0)) << "Desired tolerance must be positive.";
  }
  virtual ~IntegrationMethod() {}
  
  // Return desired tolerance.
  inline Type getTolerance() const {
    return tolerance_;
  }
  
  // Integrate function over given given interval.
  template <class Function>
  Type integrate(const Function& /*function*/, Type /*a*/, Type /*b*/) const {
    return getUndef<Type>();
  }
  
private:
  const Type tolerance_;
  
}; // IntegrationMethod

} // namespace numerical_methods

#endif // NUMERICAL_METHODS_INTEGRATION_INTEGRATION_METHOD_H_
