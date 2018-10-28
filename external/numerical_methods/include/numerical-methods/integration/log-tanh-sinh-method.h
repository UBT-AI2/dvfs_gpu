#ifndef NUMERICAL_METHODS_INTEGRATION_LOG_TANH_SINH_METHOD_H_
#define NUMERICAL_METHODS_INTEGRATION_LOG_TANH_SINH_METHOD_H_

#include <cmath>

#include <glog/logging.h>

#include "numerical-methods/common-definitions.h"
#include "numerical-methods/integration/integration-method.h"

namespace numerical_methods {

// This class implements a variation of the tanh-sinh integration method. This 
// variation computes the natural logarithm of the integral of the exponential 
// of a function within a given interval, i.e.
// 
//         /b
// y = ln | exp(f(x)) dx
//       /a
// 
// The implementation is based on the implementation of the tanh-sinh method 
// described in Bailey et al. (2005).
// 
// The logarithmic tanh-sinh method is much more accurate and numerically much 
// more stable than first computing the integral, and then the logarithm. The 
// logarithm is evaluated directly by exploiting the fact that
// 
// ln(exp(a) + exp(b)) = max(a, b) + ln(1 + exp(- abs(a - b)))
// 
// when computing the integral. This avoids underflow and overflow, and 
// preserves the loss of numerical accuracy.
// 
// D. H. Bailey, K. Jeyabalan, and X. S. Li, "A Comparison of Three High-
// precision Quadrature Schemes," in Experimental Mathematics, vol. 14, no. 3, 
// pp. 317-329 (2005).
template <typename Type>
class LogTanhSinhMethod : public IntegrationMethod<Type> {
public:
  
  explicit LogTanhSinhMethod(Type tolerance) : 
      IntegrationMethod<Type>(tolerance), order_(10) {
    initNodes();
  }
  
  LogTanhSinhMethod(Type tolerance, int order) : 
      IntegrationMethod<Type>(tolerance), order_(order) {
    CHECK_GT(order, 0) << "Order must be positive.";
    initNodes();
  }
  
  inline int getOrder() const {
    return order_;
  }
  
  // Compute the natural logarithm of the integral of the exponential of a 
  // function a over given interval.
  template <class Function>
  Type integrate(const Function& function, Type a, Type b) const {
    
    CHECK_LT(a, b) << "Interval must be non-empty.";
    
    const Type c = (b + a) / 2.0;
    const Type d = (b - a) / 2.0;
    
    std::vector<Type> values;
    values.reserve(order_);
    
    Type step = 1.0;
    Type sum = - getInf<Type>();
    Type tolerance = getUndef<Type>();
    for (int k = 0; k < order_; ++k) {
      
      // Update sum.
      const int stride = std::pow(2, order_ - k - 1);
      for (int i = 0; i < nodes_.size(); i += stride) {
        if ((k == 0) || (i % (2 * stride) != 0)) {
          const Node& node = nodes_[i];
          const Type weight = node.weight + std::log(d);
          if (node.point == 0.0) {
            sum = add(sum, function(c) + weight);
          } else {
            sum = add(sum, function(c - d * node.point) + weight);
            sum = add(sum, function(c + d * node.point) + weight);
          }
        }
      }
      
      step /= 2.0;
      values.push_back(std::log(step) + sum);
      
      // Estimate tolerance and break if desired tolerance achieved.
      tolerance = estimTolerance(values);
      if (tolerance <= this->getTolerance()) {
        return values.back();
      }
      
    }
    
    LOG_IF(WARNING, tolerance <= this->getTolerance())
        << "Maximum refinement reached. Results may be inaccurate.";
    return values.back();
    
  }
  
private:
  
  const int order_;
  
  struct Node {
    Type point;
    Type weight;
    Node(int k, Type h) {
      const Type t = k * h;
      const Type u = (getPi<Type>() / 2.0) * std::sinh(t);
      const Type v = std::cosh(u);
      point = std::sinh(u) / v;
      weight = std::log(getPi<Type>() / 2.0) + std::log(std::cosh(t)) 
          - std::log(std::pow(v, 2));
    }
  };
  
  std::vector<Node> nodes_;
  
  // Create integration rule nodes.
  void initNodes() {
    int n = 20 * std::pow(2, order_) + 1;
    const Type h = 1.0 / std::pow(2, order_);
    const Type t = std::pow(this->getTolerance(), 2);
    for (int k = 0; k < n; ++k) {
      const Node node(k, h);
      CHECK(!(isUndef(node.point) || isUndef(node.weight)));
      nodes_.push_back(node);
      if (std::abs(node.point - 1.0) <= t) {
        break;
      }
    }
  }
  
  // Estimate tolerance from previous values.
  Type estimTolerance(const std::vector<Type>& values) const {
    int i = values.size() - 1;
    if (i <= 0) {
      return 1.0;
    }
    --i;
    const Type a = std::log10(std::abs(values[i + 1] - values[i]));
    const Type b = std::log10(std::abs(values[i + 1] - values[i - 1]));
    if (isUndef<Type>(a) || isUndef<Type>(b)) {
      return this->getTolerance();
    }
    Type precision = std::max<Type>(std::pow(a, 2) / b, 2.0 * a);
    precision = std::min<Type>(std::max<Type>(precision, 
        std::log10(getEps<Type>())), 0.0);
    return std::pow(Type(0.1), - static_cast<int>(std::round(precision)));
  }
  
  static Type add(Type x, Type y) {
    return std::max(x, y) + std::log1p(std::exp(- std::abs(x - y)));
  }
  
}; // TanhSinhMethod

} // namespace numerical_methods

#endif // NUMERICAL_METHODS_INTEGRATION_LOG_TANH_SINH_METHOD_H_
