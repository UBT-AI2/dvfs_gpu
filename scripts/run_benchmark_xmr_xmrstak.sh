#!/bin/bash

device_id=$1
device_id_cuda=$2
mem_clock=$3
graph_clock=$4
log_dir=$5

if [[ "$OSTYPE" == "msys" ]]
then
#MINGW
MINER_BINARY=./miner/binaries/windows/xmr-stak-build/xmr-stak.exe
else
#Linux
MINER_BINARY=./miner/binaries/linux/xmr-stak-build/xmr-stak
fi
#################################################################################

source ./scripts/util_functions.sh
generic_bench XMR \
'${MINER_BINARY} --noCPU --benchmark 8 --benchwait 5 --benchwork 10 --cuda-devices ${device_id_cuda} --currency cryptonight_v8' \
'grep -oP "Benchmark Total:\s*\K[+-]?[0-9]+([.][0-9]+)?"'