#!/bin/bash

device_id=$1
device_id_cuda=$2
wallet_address=$3
worker_name=$4
email=$5
log_dir=$6
if [[ "$OSTYPE" == "msys" ]]
then
#MINGW
MINER_BINARY=../../miner/binaries/windows/ethminer-build/ethminer.exe
else
#Linux
MINER_BINARY=../../miner/binaries/linux/ethminer-build/ethminer
fi
LOGFILE=${log_dir}/mining_log_ethminer_gpu${device_id}.txt
#################################################################################

echo -e "\n##########################\nSTARTED ETHMINER $(date +%Y-%m-%d_%H-%M-%S)\n##########################\n" >> ${LOGFILE}
${MINER_BINARY} --farm-recheck 2000 -U -RH \
-P stratum1+tcp://${wallet_address}@eth-eu2.nanopool.org:9999/${worker_name}/${email} \
-P stratum1+tcp://${wallet_address}@eth-eu1.nanopool.org:9999/${worker_name}/${email} \
--cuda-devices ${device_id_cuda} --cuda-parallel-hash 8 --hash-logfile ${log_dir}/hash_log_ETH_${device_id}.txt &>> ${LOGFILE}

