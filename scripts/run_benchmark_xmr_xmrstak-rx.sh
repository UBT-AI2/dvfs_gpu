#!/bin/bash

device_id=$1
device_id_cuda=$2
mem_clock=$3
graph_clock=$4
log_dir=$(readlink -f $5)

if [[ "$OSTYPE" == "msys" ]]
then
#MINGW
MINER_BINARY=./miner/binaries/windows/xmr-stak-rx-build/xmr-stak-rx.exe
else
#Linux
MINER_BINARY=./miner/binaries/linux/xmr-stak-rx-build/xmr-stak-rx
fi
MINER_BINARY=$(readlink -f ${MINER_BINARY})
#################################################################################

source ./scripts/util_functions.sh
generic_bench XMR \
'${MINER_BINARY} --noCPU --noTest --benchmark 8 --benchwait 30 --benchwork 30 --cuda-devices ${device_id_cuda} --currency monero' \
'grep -oP "Benchmark Total:\s*\K[+-]?[0-9]+([.][0-9]+)?"'