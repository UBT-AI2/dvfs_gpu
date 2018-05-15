#
add_executable(calc_inner_mean
        utils/calc_inner_mean.c)

#
add_executable(gpu_power_monitor
        utils/gpu_power_monitor.c)

#
if (WIN32)
    add_executable(nvapiOC
            nvapi/main.c
            nvapi/nvapiOC.c)
endif ()

#
add_executable(nvmlOC
        nvml/main.cpp
        nvml/nvmlOC.cpp)
target_link_libraries(nvmlOC ${NVML_LIB_NAME})

#
add_executable(freq_exhaustive
        freq_exhaustive/main.cpp
        freq_exhaustive/freq_exhaustive.cpp
        nvapi/nvapiOC.c
        nvml/nvmlOC.cpp
        benchmark/benchmark.cpp)
target_link_libraries(freq_exhaustive ${NVML_LIB_NAME})

#
add_executable(freq_hill_climbing
        freq_hill_climbing/main.cpp
        freq_hill_climbing/freq_hill_climbing.cpp
        nvapi/nvapiOC.c
        nvml/nvmlOC.cpp
        benchmark/benchmark.cpp)
target_link_libraries(freq_hill_climbing ${NVML_LIB_NAME})

add_executable(freq_nelder_mead
        freq_nelder_mead/main.cpp
        freq_nelder_mead/freq_nelder_mead.cpp
        nvapi/nvapiOC.c
        nvml/nvmlOC.cpp
        benchmark/benchmark.cpp)
target_link_libraries(freq_nelder_mead ${NVML_LIB_NAME} ${GLOG_LIB_NAME})

#Specify install target for executable
#INSTALL(TARGETS ${EXECUTABLENAME} RUNTIME DESTINATION ${CMAKE_PROJECT_NAME}/bin)