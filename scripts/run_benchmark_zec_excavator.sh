#!/bin/bash

device_id=$1
device_id_cuda=$2
mem_clock=$3
graph_clock=$4
log_dir=$5

if [[ "$OSTYPE" == "msys" ]]
then
#MINGW
MINER_BINARY=./miner/binaries/windows/excavator_v1.1.0a_Win64/excavator.exe
else
#Linux
echo "exavator not available on linux"
exit 1
fi
#################################################################################

source ./scripts/util_functions.sh
generic_bench ZEC \
'${MINER_BINARY} -b -a equihash -cd ${device_id_cuda}' \
'grep -oP "Total measured:\s*\K[+-]?[0-9]+([.][0-9]+)?"'