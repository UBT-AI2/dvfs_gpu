cmake_minimum_required(VERSION 3.2)

#Set project name
project(freq_scaling)

#Specify where to put the executable
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

#Allow to organize targets into folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#Enable C++11 for g++
if (CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-sign-compare -std=c++11")
endif (CMAKE_COMPILER_IS_GNUCXX)

#Disable security warnings under visual studio
if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif (MSVC)

option(DEBUG "Enter debugging mode with more output" OFF)
#Set DEBUG as compiler flag
if (DEBUG)
    add_definitions(-DDEBUG)
endif (DEBUG)

set(DEPENDENCY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules)

#find required libs
find_package(CUDA 9.0 REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenMP REQUIRED)

#set OpenMP flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")

find_library(CUDA_DRIVER_LIBRARIES cuda
        PATHS ${CUDA_TOOLKIT_ROOT_DIR}/lib/x64)
find_path(NUMERICAL_METHODS_INCLUDE_DIRS numerical-methods/common-definitions.h
        PATHS ${DEPENDENCY_DIR}/numerical_methods/include
        NO_DEFAULT_PATH)

if(WIN32)
    #if(NOT EXISTS ${CMAKE_BINARY_DIR}/boost_1_67_0.tar.gz)
    #    file(DOWNLOAD https://sourceforge.net/projects/boost/files/boost/1.67.0/boost_1_67_0.tar.gz/download
    #            ${CMAKE_BINARY_DIR}/boost_1_67_0.tar.gz SHOW_PROGRESS)
    #endif()
    #if(NOT EXISTS ${CMAKE_BINARY_DIR}/eigen-git-mirror-3.3.5.zip)
    #    file(DOWNLOAD https://github.com/eigenteam/eigen-git-mirror/archive/3.3.5.zip
    #            ${CMAKE_BINARY_DIR}/eigen-git-mirror-3.3.5.zip SHOW_PROGRESS)
    #endif()
    #
    #execute_process(COMMAND tar xzf boost_1_67_0.tar.gz
    #        COMMAND unzip eigen-git-mirror-3.3.5.zip
    #        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    #execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory
    #            boost_1_67_0 ${CMAKE_CURRENT_SOURCE_DIR}/external/boost_1_67_0
    #        COMMAND ${CMAKE_COMMAND} -E copy_directory
    #            eigen-git-mirror-3.3.5 ${CMAKE_CURRENT_SOURCE_DIR}/external/eigen3
    #        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

    find_path(Boost_INCLUDE_DIRS boost/version.hpp
            PATHS ${DEPENDENCY_DIR}/boost_1_67_0
            NO_DEFAULT_PATH)
    find_path(EIGEN3_INCLUDE_DIR Eigen/Dense
            PATHS ${DEPENDENCY_DIR}/eigen3
            NO_DEFAULT_PATH)
    find_path(CURL_INCLUDE_DIRS curl/curl.h
            PATHS ${DEPENDENCY_DIR}/curl-7.60.0/include
            NO_DEFAULT_PATH)
    find_library(CURL_LIBRARIES libcurl
            PATHS ${DEPENDENCY_DIR}/curl-7.60.0/lib
            NO_DEFAULT_PATH)
    if(MSVC)
        find_path(GLOG_INCLUDE_DIRS glog/logging.h
                PATHS ${DEPENDENCY_DIR}/glog/msvc/include
                NO_DEFAULT_PATH)
        find_library(GLOG_LIBRARIES glog
                PATHS ${DEPENDENCY_DIR}/glog/msvc/lib
                NO_DEFAULT_PATH)
    else(MSVC)
        find_path(GLOG_INCLUDE_DIRS glog/logging.h
                PATHS ${DEPENDENCY_DIR}/glog/mingw/include
                NO_DEFAULT_PATH)
        find_library(GLOG_LIBRARIES libglog
                PATHS ${DEPENDENCY_DIR}/glog/mingw/lib
                NO_DEFAULT_PATH)
    endif(MSVC)
    find_library(NVML_LIBRARIES nvml
            PATHS ${CUDA_TOOLKIT_ROOT_DIR}/lib/x64
            NO_DEFAULT_PATH)
else(WIN32)
    find_package(Boost 1.58.0 REQUIRED)
    find_package(EIGEN3 REQUIRED)
    find_package(CURL REQUIRED)
    find_path(GLOG_INCLUDE_DIRS glog/logging.h)
    find_library(GLOG_LIBRARIES glog)
    #link nvml
    execute_process(
            COMMAND bash -c "nvidia-smi | grep Driver | cut -f 3 -d' ' | cut -f 1 -d '.'"
            OUTPUT_VARIABLE DRIVER-BRANCH
    )
    string(STRIP ${DRIVER-BRANCH} DRIVER-BRANCH)
    find_library(NVML_LIBRARIES nvidia-ml
            PATHS /usr/lib/nvidia-${DRIVER-BRANCH})
    find_path(XNVCTRL_INCLUDE_DIRS NVCtrl.h
            PATHS ${DEPENDENCY_DIR}/libXNVCtrl
            NO_DEFAULT_PATH)
    find_library(XNVCTRL_LIBRARIES XNVCtrl
            PATHS ${DEPENDENCY_DIR}/libXNVCtrl
            NO_DEFAULT_PATH)
endif(WIN32)


#include and link cuda toolkit headers/libs
include_directories(${CUDA_TOOLKIT_ROOT_DIR}/include)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${EIGEN3_INCLUDE_DIR})
include_directories(${CURL_INCLUDE_DIRS})
include_directories(${GLOG_INCLUDE_DIRS})
include_directories(${NUMERICAL_METHODS_INCLUDE_DIRS})
if (NOT WIN32)
    include_directories(${XNVCTRL_INCLUDE_DIRS})
endif(NOT WIN32)



# Glob all sources file inside directory ${DIRECTORY}
# extract relative path of all source files
file(GLOB_RECURSE FREQ_SCALING_SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
        )

#generates source folder structure in build directory from relative path extracted above
foreach (f ${FREQ_SCALING_SOURCE_FILES})
    # Get the path of the file relative to ${DIRECTORY},
    # then alter it (not compulsory)
    file(RELATIVE_PATH SRCGR ${CMAKE_CURRENT_SOURCE_DIR} ${f})
    # Extract the folder, ie remove the filename part
    string(REGEX REPLACE "(.*)(/[^/]*)$" "\\1" SRCGR ${SRCGR})
    # Source_group expects \\ (double antislash), not / (slash)
    string(REPLACE / \\ SRCGR ${SRCGR})
    source_group("${SRCGR}" FILES ${f})
endforeach ()

#######################################################################################

add_subdirectory(src)

