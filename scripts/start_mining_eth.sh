#!/bin/bash

device_id=$1
wallet_address=$2
worker_name=$3
email=$4
if [[ "$OSTYPE" == "msys" ]]
then
#MINGW
MINER_BINARY=../../miner/binaries/windows/ethminer-build/ethminer.exe
else
#Linux
MINER_BINARY=../../miner/binaries/linux/ethminer-build/ethminer
fi
#################################################################################

${MINER_BINARY} --farm-recheck 200 -U -S eth-eu1.nanopool.org:9999 -FS eth-eu2.nanopool.org:9999 \
-O ${wallet_address}.${worker_name}/${email} --cuda-devices $device_id --cuda-parallel-hash 8

