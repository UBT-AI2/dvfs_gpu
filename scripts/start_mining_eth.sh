#!/bin/bash

device_id=$1
device_id_cuda=$2
wallet_address=$3
worker_name=$4
email=$5
if [[ "$OSTYPE" == "msys" ]]
then
#MINGW
MINER_BINARY=../../miner/binaries/windows/ethminer-build/ethminer.exe
else
#Linux
MINER_BINARY=../../miner/binaries/linux/ethminer-build/ethminer
fi
#################################################################################

${MINER_BINARY} --farm-recheck 2000 -U \
-P stratum+tcp://${wallet_address}@eu1.nanopool.org:9999/${worker_name}/${email} \
--cuda-devices ${device_id_cuda} --cuda-parallel-hash 8

