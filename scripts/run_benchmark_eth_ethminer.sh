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

source $(dirname $(readlink -f $0))/util_functions.sh
generic_bench ETH \
'${MINER_BINARY} -U -M --benchmark-trials 1 --benchmark-warmup 15 --cuda-devices ${device_id_cuda}' \
'grep -oP "Hashrate 1:\s*\K[+-]?[0-9]+([.][0-9]+)?"'


