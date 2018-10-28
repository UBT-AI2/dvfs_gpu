cmake_minimum_required(VERSION 2.8.3)
project(numerical_methods)

find_package(catkin_simple REQUIRED)
catkin_simple(ALL_DEPS_REQUIRED)

add_definitions(-std=c++11)

cs_add_library(${PROJECT_NAME} src/numerical-methods.cc)

# Integration methods.
catkin_add_gtest(test_integration_methods 
  test/test-integration-methods.cc
)
target_link_libraries(test_integration_methods 
  ${PROJECT_NAME} 
  pthread
)

# Log-integration methods.
catkin_add_gtest(test_log_integration_methods 
  test/test-log-integration-methods.cc
)
target_link_libraries(test_log_integration_methods 
  ${PROJECT_NAME} 
  pthread
)

# Optimization methods.
catkin_add_gtest(test_optimization_methods 
  test/test-optimization-methods.cc
)
target_link_libraries(test_optimization_methods 
  ${PROJECT_NAME} 
  pthread
)

# Incremental statistics utility.
catkin_add_gtest(test_incremental_statistics 
  test/test-incremental-statistics.cc
)
target_link_libraries(test_incremental_statistics 
  ${PROJECT_NAME} 
  pthread
)

# Linear model utility.
catkin_add_gtest(test_linear_model 
  test/test-linear-model.cc
)
target_link_libraries(test_linear_model 
  ${PROJECT_NAME} 
  pthread
)

cs_install()
cs_export()
