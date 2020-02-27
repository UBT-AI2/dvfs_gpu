#!/bin/bash

device_id=$1
device_id_cuda=$2
mem_clock=$3
graph_clock=$4
log_dir=$5

if [[ "$OSTYPE" == "msys" ]]
then
#MINGW
MINER_BINARY=./miner/binaries/windows/ethminer-build/ethminer.exe
else
#Linux
MINER_BINARY=./miner/binaries/linux/ethminer-build/ethminer
fi
#################################################################################

source ./scripts/util_functions.sh
generic_bench ETH \
'${MINER_BINARY} -U -M 7 --cuda-devices ${device_id_cuda} --mining-duration 30' \
'grep -oP "Hashrate:\s*\K[+-]?[0-9]+([.][0-9]+)?"'


