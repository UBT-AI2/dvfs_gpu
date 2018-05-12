
#include and link cuda toolkit headers/libs
include_directories(${CUDA_TOOLKIT_ROOT_DIR}/include)
link_directories(${CUDA_TOOLKIT_ROOT_DIR}/lib/x64)
#
add_executable(calc_inner_mean utils/calc_inner_mean.c)
add_executable(gpu_power_monitor utils/gpu_power_monitor.c)

if(WIN32)
    add_executable(nvapiOC nvapi/main.c nvapi/nvapiOC.c)
endif()
add_executable(nvmlOC nvml/main.cpp nvml/nvmlOC.cpp)
target_link_libraries(nvmlOC nvml)
add_executable(freq_benchmark freq_benchmark.cpp nvapi/nvapiOC.c nvml/nvmlOC.cpp)
target_link_libraries(freq_benchmark nvml)
add_executable(freq_hill_climbing freq_hill_climbing.cpp nvapi/nvapiOC.c nvml/nvmlOC.cpp)
target_link_libraries(freq_hill_climbing nvml)

#Specify install target for executable
#INSTALL(TARGETS ${EXECUTABLENAME} RUNTIME DESTINATION ${CMAKE_PROJECT_NAME}/bin)