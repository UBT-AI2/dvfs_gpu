
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

#find required libs
find_package(CUDA 9.0 REQUIRED)
find_package(OpenMP REQUIRED)
find_package(Threads REQUIRED)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules)
find_package(EIGEN3 REQUIRED)
find_package(Boost REQUIRED)
if(WIN32)
    STRING(REPLACE "\\" "/" CURL_DIR2 $ENV{CURL_DIR})
    STRING(REPLACE "\ " "\\ " CURL_DIR2 ${CURL_DIR2})
    set(CURL_INCLUDE_DIRS ${CURL_DIR2}/include)
    set(CURL_LIBRARIES "${CURL_DIR2}/lib/libcurl.lib")
else(WIN32)
    find_package(CURL REQUIRED)
endif(WIN32)

#include and link cuda toolkit headers/libs
include_directories(${CUDA_TOOLKIT_ROOT_DIR}/include)
include_directories(${EIGEN3_INCLUDE_DIR})
include_directories(${CURL_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/numerical_methods/include)

if (WIN32)
	if(MSVC)
		include_directories($ENV{GLOG_DIR}/vs/include)
		link_directories($ENV{GLOG_DIR}/vs/lib)
		set(GLOG_LIB_NAME glog)
	else(MSVC)
		include_directories($ENV{GLOG_DIR}/include)
		link_directories($ENV{GLOG_DIR}/lib)
		set(GLOG_LIB_NAME libglog)
	endif(MSVC)
    link_directories(${CUDA_TOOLKIT_ROOT_DIR}/lib/x64)
    set(NVML_LIB_NAME nvml)
else (WIN32)
    #link nvml
    #execute_process(
    #        COMMAND bash -c "nvidia-smi | grep Driver | cut -f 3 -d' ' | cut -f 1 -d '.'"
    #        OUTPUT_VARIABLE DRIVER-BRANCH
    #)
    #message(${DRIVER-BRANCH})
    link_directories(/usr/lib/nvidia-387)
    #include and link XNVCtrl
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/libXNVCtrl)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/libXNVCtrl)
    #
    set(XNVCTRL_LIB_NAME XNVCtrl)
    set(NVML_LIB_NAME nvidia-ml)
    set(GLOG_LIB_NAME glog)
endif (WIN32)

#set OpenMP flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")


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

