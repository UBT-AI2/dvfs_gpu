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
MINER_BINARY=../../miner/binaries/windows/ccminer-build/ccminer.exe
else
#Linux
MINER_BINARY=../../miner/binaries/linux/ccminer-build/ccminer
fi
LOGFILE=${log_dir}/mining_log_ccminer-lux_gpu${device_id}.txt
#################################################################################

echo -e "\n##########################\nSTARTED CCMINER-LUX $(date +%Y-%m-%d_%H-%M-%S)\n##########################\n" >> ${LOGFILE}
${MINER_BINARY} -a phi2 -o stratum+tcp://omegapool.cc:8003 -u ${wallet_address}.${worker_name} \
-p x -D -d ${device_id_cuda} --hash-log=${log_dir}/hash_log_LUX_${device_id}.txt &>> ${LOGFILE}
