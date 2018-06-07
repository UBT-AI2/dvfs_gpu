#!/bin/bash

device_id=$1
device_id_cuda=$2
wallet_address=$3
worker_name=$4
email=$5
if [[ "$OSTYPE" == "msys" ]]
then
#MINGW
MINER_BINARY=../../miner/binaries/windows/excavator_v1.1.0a_Win64/excavator.exe
else
#Linux
echo "exavator not available on linux"
exit 1
fi
#################################################################################

${MINER_BINARY} -a equihash -s zec-eu1.nanopool.org:6666 -u ${wallet_address}/${worker_name}/${email} -p 0 -d 2 -cd $device_id_cuda
