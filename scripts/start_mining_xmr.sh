#!/bin/bash

device_id=$1
wallet_address=$2
worker_name=$3
email=$4
if [[ "$OSTYPE" == "msys" ]]
then
#MINGW
MINER_BINARY=../../miner/binaries/windows/xmr-stak-build/xmr-stak.exe
else
#Linux
MINER_BINARY=../../miner/binaries/linux/xmr-stak-build/xmr-stak
fi
#################################################################################

${MINER_BINARY} --noCPU --nvidia nvidia$device_id.txt \
-O xmr-eu1.nanopool.org:14433 -u ${wallet_address}.${worker_name}/${email} --currency monero7 -i 0 -p "" -r ""
