# dvfs_gpu
Autotuning framework with DVFS on GPUs for the mining of cryptocurrencies.

# Installation guide
### Framework - Windows
To build the framework under Windows the following steps are necessary:
1. Install Visual Studio 2015 or newer. Alternatively use MinGW 5 or newer. In the case of MinGW add the path to the binaries (*gcc.exe*, *g++.exe*, etc.) to the system environment variable PATH and rename *mingw32-make.exe* in *make.exe*. With MinGW a suitable IDE is CLion or the Qt Creator, because of the built-in CMake-Support.
2. Install Cuda Toolkit 9.0 or newer as well as the latest NVIDIA-Treiber. Add the path to the binaries (*nvcc.exe*, *nvidia-smi.exe*, etc.) to the system envionment variable PATH.
3. Install CMake 3.2 or newer. Add the path to the binaries (*cmake.exe*) to the system envionment variable PATH.
4. Install Git for Windows. During the selection of the shell in the installer choose the Windows-Shell, not the preconfigured MinTTY-Shell. Add the path to the binaries (*git.exe*, *bash.exe* und *sh.exe*) to the system envionment variable PATH.
5. Checkout the repository at the desired location with `git clone https://github.com/UBT-AI2/dvfs_gpu.git`.
6. In the directory of the Top-Level-CMakeLists.txt execute the following:
    * `mkdir build && cd build`
    * `cmake -G "MinGW Makefiles" ..` for MinGW or `cmake ..` for Visual Studio.
    * `make` for MinGW or open generated Visual Studio Solution and build project.
7. Alternatively directly open the Top-Level-CMakeLists.txt in an IDE with CMake-Support (Visual Studio 2017, CLion, Qt Creator) and build the project.

### Framework - Linux
To build the framework under Linux the following steps are necessary:
1. Install Cuda Toolkit 9.0 or newer as well as the latest NVIDIA-Treiber.
2. Install dependencies with the script `intstall_linux_dependencies.sh` (folder utils).
3. Checkout the repository at the desired location with `git clone https://github.com/UBT-AI2/dvfs_gpu.git`.
4. In the directory of the Top-Level-CMakeLists.txt execute the following:
    * `mkdir build && cd build`
    * `cmake ..`
    * `make`

### Miner
For the default used currencies of the framework there are prebuild binaries of mining software available for Windows and Linux. To rebuild the miners with available source code execute the script `init-submodules.sh` (folder utils/submodule-stuff). This script initializes the Git-Submodules in the miner directory and applies the available patches to them. For a guide to build the individual miners see the coresponding repositorities of the miners.
