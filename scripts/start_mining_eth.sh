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
LOGFILE=mining_log_ethminer_gpu${device_id}_$(date +%Y-%m-%d_%H-%M-%S).txt
#################################################################################

echo -e "\n##########################\n$(date +%Y-%m-%d_%H-%M-%S)\n##########################\n" >> ${LOGFILE}
${MINER_BINARY} --farm-recheck 2000 -U -RH \
-P stratum1+tcp://${wallet_address}@eth-eu1.nanopool.org:9999/${worker_name}/${email} \
--cuda-devices ${device_id_cuda} --cuda-parallel-hash 8 --hash-logfile hash_log_ETH_${device_id}.txt &>> ${LOGFILE}

