set(COMMON_HEADER
        common_header/constants.h
        common_header/exceptions.h
        common_header/fullexpr_accum.h)

set(NVAPI_FILES
        nvapi/nvapiOC.h nvapi/nvapiOC.cpp
        ${COMMON_HEADER})
set(NVML_FILES
        nvml/nvmlOC.h nvml/nvmlOC.cpp
        ${COMMON_HEADER})
set(SCRIPT_RUNNING_FILES
        script_running/currency_type.h script_running/currency_type.cpp
        script_running/benchmark.h script_running/benchmark.cpp
        script_running/mining.h script_running/mining.cpp
        script_running/process_management.h script_running/process_management.cpp
        script_running/log_utils.h script_running/log_utils.cpp
        script_running/network_requests.h script_running/network_requests.cpp
        script_running/optimization_config.h script_running/optimization_config.cpp
        script_running/cli_utils.h script_running/cli_utils.cpp
        ${COMMON_HEADER})
set(FREQ_EXHAUSTIVE_FILES
        freq_exhaustive/freq_exhaustive.h freq_exhaustive/freq_exhaustive.cpp
        ${COMMON_HEADER})
set(FREQ_OPTIMIZATION_FILES
        freq_optimization/freq_simulated_annealing.h freq_optimization/freq_simulated_annealing.cpp
        freq_optimization/freq_nelder_mead.h freq_optimization/freq_nelder_mead.cpp
        freq_optimization/freq_hill_climbing.h freq_optimization/freq_hill_climbing.cpp
        ${COMMON_HEADER})
set(PROFIT_OPTIMIZATION_FILES
        profit_optimization/profit_calculation.h profit_optimization/profit_calculation.cpp
        profit_optimization/profit_optimization.h profit_optimization/profit_optimization.cpp
        ${COMMON_HEADER})

#
add_library(nvapiOC_lib
        ${NVAPI_FILES})
if (NOT WIN32)
    target_link_libraries(nvapiOC_lib
            ${XNVCTRL_LIBRARIES}
            Xext
            X11)
endif ()
add_executable(nvapiOC
        nvapi/main.cpp)
target_link_libraries(nvapiOC
        nvapiOC_lib
        ${GLOG_LIBRARIES})

#
add_library(nvmlOC_lib
        ${NVML_FILES})
target_link_libraries(nvmlOC_lib
        ${NVML_LIBRARIES})
add_executable(nvmlOC
        nvml/main.cpp)
target_link_libraries(nvmlOC
        nvmlOC_lib
        ${GLOG_LIBRARIES})

#
add_library(script_running_lib
        ${SCRIPT_RUNNING_FILES})
target_link_libraries(script_running_lib
            nvapiOC_lib
            nvmlOC_lib
            ${CUDA_DRIVER_LIBRARIES}
            ${CURL_LIBRARIES}
            ${GLOG_LIBRARIES})

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
add_library(freq_optimization_lib
        ${FREQ_OPTIMIZATION_FILES})
target_link_libraries(freq_optimization_lib
        script_running_lib)
add_executable(freq_optimization
        freq_optimization/main.cpp)
target_link_libraries(freq_optimization
        freq_optimization_lib)

#
add_library(profit_optimization_lib
        ${PROFIT_OPTIMIZATION_FILES})
target_link_libraries(profit_optimization_lib
        freq_optimization_lib)
add_executable(profit_optimization
        profit_optimization/main.cpp)
target_link_libraries(profit_optimization
        profit_optimization_lib)


#
add_executable(gpu_power_monitor
        utils/gpu_power_monitor.cpp)
target_link_libraries(gpu_power_monitor
        nvmlOC_lib
        ${GLOG_LIBRARIES})
add_executable(hash-log-excavator
        utils/hash-log-excavator.cpp)


#Specify install target for executable
#INSTALL(TARGETS ${EXECUTABLENAME} RUNTIME DESTINATION ${CMAKE_PROJECT_NAME}/bin)
if (WIN32)
    file(COPY ${DEPENDENCY_DIR}/dlls/ DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            FILES_MATCHING PATTERN "*.dll")
endif (WIN32)