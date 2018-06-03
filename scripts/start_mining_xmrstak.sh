#!/bin/bash

user_data=$1
device_id=$2
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
-O xmr-eu1.nanopool.org:14433 -u $user_data --currency monero7 -i 0 -p "" -r ""
