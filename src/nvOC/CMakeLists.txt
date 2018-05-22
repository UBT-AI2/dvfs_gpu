
set(NVAPI_FILES
        nvapi/nvapiOC.h nvapi/nvapiOC.c)
set(NVML_FILES
        nvml/nvmlOC.h nvml/nvmlOC.cpp)
set(SCRIPT_RUNNING_FILES
        script_running/miner_script.h
        script_running/benchmark.h script_running/benchmark.cpp
        script_running/mining.h script_running/mining.cpp)
set(FREQ_EXHAUSTIVE_FILES
        freq_exhaustive/freq_exhaustive.h freq_exhaustive/freq_exhaustive.cpp)
set(FREQ_HILL_CLIMBING_FILES
        freq_hill_climbing/freq_hill_climbing.h freq_hill_climbing/freq_hill_climbing.cpp)
set(FREQ_NELDER_MEAD_FILES
        freq_nelder_mead/freq_nelder_mead.h freq_nelder_mead/freq_nelder_mead.cpp)
set(PROFIT_OPTIMIZATION_FILES
        profit_optimization/profit_calculation.h profit_optimization/profit_calculation.cpp
        profit_optimization/profit_optimization.h profit_optimization/profit_optimization.cpp)


#
add_executable(calc_inner_mean
        utils/calc_inner_mean.c)

#
add_executable(gpu_power_monitor
        utils/gpu_power_monitor.c)

#
add_library(nvapiOC_lib
        ${NVAPI_FILES})
add_executable(nvapiOC
        nvapi/main.c)
target_link_libraries(nvapiOC
        nvapiOC_lib)

#
add_library(nvmlOC_lib
        ${NVML_FILES})
target_link_libraries(nvmlOC_lib
        ${NVML_LIB_NAME})
add_executable(nvmlOC
        nvml/main.cpp)
target_link_libraries(nvmlOC
        nvmlOC_lib)

#
add_library(script_running_lib
        ${SCRIPT_RUNNING_FILES})
target_link_libraries(script_running_lib
        nvapiOC_lib
        nvmlOC_lib)

#
add_library(freq_exhaustive_lib
        ${FREQ_EXHAUSTIVE_FILES})
target_link_libraries(freq_exhaustive_lib
        script_running_lib)
add_executable(freq_exhaustive
        freq_exhaustive/main.cpp)
target_link_libraries(freq_exhaustive
        freq_exhaustive_lib)

#
add_library(freq_hill_climbing_lib
        ${FREQ_HILL_CLIMBING_FILES})
target_link_libraries(freq_hill_climbing_lib
        script_running_lib)
add_executable(freq_hill_climbing
        freq_hill_climbing/main.cpp)
target_link_libraries(freq_hill_climbing
        freq_hill_climbing_lib)

#
add_library(freq_nelder_mead_lib
        ${FREQ_NELDER_MEAD_FILES})
target_link_libraries(freq_nelder_mead_lib
        script_running_lib
        ${GLOG_LIB_NAME})
add_executable(freq_nelder_mead
        freq_nelder_mead/main.cpp)
target_link_libraries(freq_nelder_mead
        freq_nelder_mead_lib)

#
add_library(profit_optimization_lib
        ${PROFIT_OPTIMIZATION_FILES})
target_link_libraries(profit_optimization_lib
        freq_hill_climbing_lib
        freq_nelder_mead_lib)
add_executable(profit_optimization
        profit_optimization/main.cpp)
target_link_libraries(profit_optimization
        profit_optimization_lib)


#Specify install target for executable
#INSTALL(TARGETS ${EXECUTABLENAME} RUNTIME DESTINATION ${CMAKE_PROJECT_NAME}/bin)