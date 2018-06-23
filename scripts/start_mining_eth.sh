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
LOGFILE=ethminer_log_gpu${device_id}.txt
#################################################################################

echo -e "\n##########################\n$(date +%Y-%m-%d_%H-%M-%S)\n##########################\n" >> ${LOGFILE}
${MINER_BINARY} --farm-recheck 2000 -U -RH \
-P stratum+tcp://${wallet_address}.${worker_name}@eth-eu1.nanopool.org:9999 -SE ${email} \
--cuda-devices ${device_id_cuda} --cuda-parallel-hash 8 &>> ${LOGFILE}

