cmake_minimum_required(VERSION 3.2)

#Set project name
project(freq_scaling)

#Allow to organize targets into folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#Specify where to put the executable
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    message(WARNING "No install directory was given using default: ${CMAKE_SOURCE_DIR}/install")
    set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install CACHE PATH "..." FORCE)
endif (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
message(STATUS "Install path: ${CMAKE_INSTALL_PREFIX}")

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


# Download automatically, you can also just copy the conan.cmake file
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
   message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
   file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.14/conan.cmake"
                 "${CMAKE_BINARY_DIR}/conan.cmake")
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)
conan_add_remote(NAME mining INDEX 1
            URL https://api.bintray.com/conan/alexfiebig15/mining)
conan_cmake_run(CONANFILE conan/conanfile.py
                BASIC_SETUP CMAKE_TARGETS
                BUILD missing)

#execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/conan/conan_install.sh ${CMAKE_SOURCE_DIR}/conan)
#include(${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/conanbuildinfo.cmake)
#conan_basic_setup(TARGETS)

#find required libs
find_package(CUDA 9.0 REQUIRED)

#find cuda driver lib
find_library(CUDA_DRIVER_LIBRARIES cuda
        PATHS ${CUDA_TOOLKIT_ROOT_DIR}/lib/x64)

#find nvml lib
if(WIN32)
    find_library(NVML_LIBRARIES nvml
            PATHS ${CUDA_TOOLKIT_ROOT_DIR}/lib/x64
            NO_DEFAULT_PATH)
else(WIN32)
    execute_process(
            COMMAND bash -c "nvidia-smi | grep Driver | cut -f 3 -d' ' | cut -f 1 -d '.'"
            OUTPUT_VARIABLE DRIVER-BRANCH
    )
    if(${DRIVER-BRANCH})
        string(STRIP ${DRIVER-BRANCH} DRIVER-BRANCH)
    endif()
    find_library(NVML_LIBRARIES nvidia-ml
            PATHS /usr/lib/nvidia-${DRIVER-BRANCH})
endif(WIN32)


#include and link cuda toolkit headers/libs
include_directories(${CUDA_TOOLKIT_ROOT_DIR}/include)
include_directories(${CONAN_INCLUDE_DIRS})



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

