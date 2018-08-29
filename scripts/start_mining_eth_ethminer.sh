#!/bin/bash

device_id=$1
device_id_cuda=$2
wallet_address=$3
worker_name=$4
email=$5
log_dir=$6
pool_csv=$7
pool_pass=$8
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

pool_list=($(echo ${pool_csv} | tr "," "\n"))
for pool in "${pool_list[@]}"
do
    pool_option_str="${pool_option_str} -P stratum1+tcp://${wallet_address}:${pool_pass}@${pool}/${worker_name}/${email}"
done

echo -e "\n##########################\nSTARTED ETHMINER $(date +%Y-%m-%d_%H-%M-%S)\n##########################\n" >> ${LOGFILE}
${MINER_BINARY} --farm-recheck 2000 -U -RH ${pool_option_str} \
--cuda-devices ${device_id_cuda} --cuda-parallel-hash 8 --hash-logfile ${log_dir}/hash_log_ETH_${device_id}.txt &>> ${LOGFILE}

