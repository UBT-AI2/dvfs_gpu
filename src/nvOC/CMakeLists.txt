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
set(FREQ_CORE_FILES
        freq_core/currency_type.h freq_core/currency_type.cpp
        freq_core/benchmark.h freq_core/benchmark.cpp
        freq_core/mining.h freq_core/mining.cpp
        freq_core/process_management.h freq_core/process_management.cpp
        freq_core/log_utils.h freq_core/log_utils.cpp
        freq_core/network_requests.h freq_core/network_requests.cpp
        freq_core/optimization_config.h freq_core/optimization_config.cpp
        freq_core/cli_utils.h freq_core/cli_utils.cpp
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

#NVAPI
add_library(nvapiOC_lib
        ${NVAPI_FILES})
target_link_libraries(nvapiOC_lib
        CONAN_PKG::glog)
if (NOT WIN32)
    target_link_libraries(nvapiOC_lib
            CONAN_PKG::libXNVCtrl
            Xext
            X11)
endif (NOT WIN32)
add_executable(nvapiOC
        nvapi/main.cpp)
target_link_libraries(nvapiOC
        nvapiOC_lib)

#NVML
add_library(nvmlOC_lib
        ${NVML_FILES})
target_link_libraries(nvmlOC_lib
        ${NVML_LIBRARIES}
        CONAN_PKG::glog)
add_executable(nvmlOC
        nvml/main.cpp)
target_link_libraries(nvmlOC
        nvmlOC_lib)

#FREQ_CORE
add_library(freq_core_lib
        ${FREQ_CORE_FILES})
target_link_libraries(freq_core_lib
        nvapiOC_lib
        nvmlOC_lib
        ${CUDA_DRIVER_LIBRARIES}
        CONAN_PKG::libcurl)

#FREQ_EXHAUSTIVE
add_library(freq_exhaustive_lib
        ${FREQ_EXHAUSTIVE_FILES})
target_link_libraries(freq_exhaustive_lib
        freq_core_lib)
add_executable(freq_exhaustive
        freq_exhaustive/main.cpp)
target_link_libraries(freq_exhaustive
        freq_exhaustive_lib)

#FREQ_OPTIMIZATION
add_library(freq_optimization_lib
        ${FREQ_OPTIMIZATION_FILES})
target_link_libraries(freq_optimization_lib
        freq_core_lib)
add_executable(freq_optimization
        freq_optimization/main.cpp)
target_link_libraries(freq_optimization
        freq_optimization_lib)

#PROFIT_OPTIMIZATION
add_library(profit_optimization_lib
        ${PROFIT_OPTIMIZATION_FILES})
target_link_libraries(profit_optimization_lib
        freq_optimization_lib)
add_executable(profit_optimization
        profit_optimization/main.cpp)
target_link_libraries(profit_optimization
        profit_optimization_lib)

#POWER_MONITOR
add_executable(gpu_power_monitor
        utils/gpu_power_monitor.cpp)
target_link_libraries(gpu_power_monitor
        nvmlOC_lib)
#HASH_LOG_EXCAVATOR
add_executable(hash-log-excavator
        utils/hash-log-excavator.cpp)

#COPY NECESSARY FILES TO BINARY DIR
file(COPY ${CMAKE_SOURCE_DIR}/scripts DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(COPY ${CMAKE_SOURCE_DIR}/configs DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
if (WIN32)
    file(COPY ${CMAKE_SOURCE_DIR}/miner/binaries/windows DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miner/binaries)
else(WIN32)
    file(COPY ${CMAKE_SOURCE_DIR}/miner/binaries/linux DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miner/binaries)
endif(WIN32)


#INSTALL STUFF
#Specify install target for executables
INSTALL(TARGETS nvapiOC nvmlOC freq_exhaustive freq_optimization profit_optimization gpu_power_monitor hash-log-excavator
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
#Specify install target for libs
INSTALL(TARGETS nvapiOC_lib nvmlOC_lib freq_core_lib freq_exhaustive_lib freq_optimization_lib profit_optimization_lib
        ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

#Specify install target for headers
set(HEADER_DIRS common_header freq_exhaustive freq_optimization nvapi nvml profit_optimization freq_core)
foreach (dir ${HEADER_DIRS})
    INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/
            DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/${dir}
            FILES_MATCHING PATTERN "*.h")
endforeach ()

#install necessary files
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/configs DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/scripts DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
        USE_SOURCE_PERMISSIONS)
if (WIN32)
    INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/miner/binaries/windows DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/miner/binaries
            USE_SOURCE_PERMISSIONS)
	INSTALL(DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
            USE_SOURCE_PERMISSIONS FILES_MATCHING PATTERN "*.dll")
else(WIN32)
    INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/miner/binaries/linux DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/miner/binaries
            USE_SOURCE_PERMISSIONS)
	INSTALL(DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
            USE_SOURCE_PERMISSIONS FILES_MATCHING PATTERN "*.so*")
endif (WIN32)