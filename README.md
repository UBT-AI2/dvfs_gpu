# dvfs_gpu
An autotuning framework using DVFS on GPUs for the mining of cryptocurrencies. The framework consists of a main program *profit_opitmization* and several other programs representing single components of the main program. The individual programs are:
* **profit_optimization:** Main program with frequency optimization and calculation/monitoring of energy costs and mining earnings. GPUs, currencies and optimization algorithms that should be used are configurable through JSON files. The available command line options with a short description are listed when executing `profit_optimization --help`. 
* **freq_optimization:** Optimization of frequencies with specified algorithm for a specified currency on a specified GPU. Offline as well as online optimization possible. The available command line options with a short description are listed when executing `freq_optimization --help`.
* **freq_exhaustive:** Testing of all configurable frequencies on a specified GPU for a specified currency. Offline as well as online benchmarks possible. The available command line options with a short description are listed when executing `freq_exhaustive --help`.
* **nvmlOC:** Setting of frequencies on a specified GPU using the NVML library. An usage message is displayed when executing `nvmlOC`. 
* **nvapiOC:** Setting of frequencies on a specified GPU using the NVAPI (Windows) or NV-Control X (Linux) library. An usage message is displayed when executing `nvapiOC`.

# Usage notes
To be able to change GPU frequencies the framework requires root (Linux) or administrator (Windows) privileges. When using the framework under Linux the coolbits option in the X-Configuration (/etc/X11/xorg.conf) has to be set for each GPU to use (see [here](https://gist.github.com/bsodmike/369f8a202c5a5c97cfbd481264d549e9)). Moreover, when the Linux computer is used remotely over `ssh` the script `start_remote_X.sh` (folder utils) must executed with the `source` command before program start. This script stops the running X-Server and starts a new one on the display of the remote computer.

# Add new currency
To make a new currency available for usage with the framework a corresponding section has to be added to the currency configuration. At program start the modified currency configuration has to be specified using the corresponding command line option. If no currency configuration is specified a default one is created and used. 

# Installation guide
### Framework - Windows
To build the framework under Windows the following steps are necessary:
1. Install [Visual Studio](https://visualstudio.microsoft.com/) 2015 or newer. Alternatively use [MinGW](https://sourceforge.net/projects/mingw-w64/) 5 or newer. In the case of MinGW add the path to the binaries (*gcc.exe*, *g++.exe*, etc.) to the system environment variable PATH and rename *mingw32-make.exe* in *make.exe*. With MinGW a suitable IDE is CLion or the Qt Creator, because of the built-in CMake-Support.
2. Install [Cuda Toolkit](https://developer.nvidia.com/cuda-downloads) 9.0 or newer as well as the latest [NVIDIA driver](https://www.nvidia.de/Download/index.aspx?lang=en). Add the path to the binaries (*nvcc.exe*, *nvidia-smi.exe*, etc.) to the system environment variable PATH.
3. Install [CMake](https://cmake.org/) 3.2 or newer. Add the path to the binaries (*cmake.exe*) to the system environment variable PATH.
4. Install [Git for Windows](https://gitforwindows.org/). During the selection of the shell in the installer choose the Windows-Shell, not the preconfigured MinTTY-Shell. Add the path to the binaries (*git.exe*, *bash.exe* and *sh.exe*) to the system environment variable PATH.
5. Checkout the repository at the desired location with `git clone https://github.com/UBT-AI2/dvfs_gpu.git`.
6. In the directory of the Top-Level-CMakeLists.txt execute the following:
    * `mkdir build && cd build`
    * `cmake -G "MinGW Makefiles" ..` for MinGW or `cmake ..` for Visual Studio.
    * `make` for MinGW or open generated Visual Studio Solution and build project.
7. Alternatively directly open the Top-Level-CMakeLists.txt in an IDE with CMake-Support (Visual Studio 2017, CLion, Qt Creator) and build the project.

### Framework - Linux
To build the framework under Linux the following steps are necessary:
1. Install [Cuda Toolkit](https://developer.nvidia.com/cuda-downloads) 9.0 or newer as well as the latest [NVIDIA driver](https://www.nvidia.de/Download/index.aspx?lang=en). Make sure the g++ compiler version 5 or newer is installed.
2. Install dependencies with the script `intstall_linux_dependencies.sh` (folder utils).
3. Checkout the repository at the desired location with `git clone https://github.com/UBT-AI2/dvfs_gpu.git`.
4. In the directory of the Top-Level-CMakeLists.txt execute the following:
    * `mkdir build && cd build`
    * `cmake ..`
    * `make`

### Miner
For the default used currencies of the framework there are prebuilt binaries of mining software available for Windows and Linux. To rebuild the miners with available source code execute the script `init-submodules.sh` (folder utils/submodule-stuff). This script initializes the Git-Submodules in the miner directory and applies the available patches to them. For a guide to build the individual miners see the corresponding repositorities of the miners.
