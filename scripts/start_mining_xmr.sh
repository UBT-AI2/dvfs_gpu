#!/bin/bash

device_id=$1
device_id_cuda=$2
wallet_address=$3
worker_name=$4
email=$5
if [[ "$OSTYPE" == "msys" ]]
then
#MINGW
MINER_BINARY=../../miner/binaries/windows/xmr-stak-build/xmr-stak.exe
else
#Linux
MINER_BINARY=../../miner/binaries/linux/xmr-stak-build/xmr-stak
fi
#################################################################################

${MINER_BINARY} --noCPU --nvidia nvidia${device_id}.txt \
-O xmr-eu1.nanopool.org:14433 -u ${wallet_address}.${worker_name}/${email} --currency monero7 -i 0 -p "" -r ""
