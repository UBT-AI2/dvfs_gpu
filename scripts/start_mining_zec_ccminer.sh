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
LOGFILE=${log_dir}/mining_log_ccminer-zec_gpu${device_id}.txt
#################################################################################

echo -e "\n##########################\nSTARTED CCMINER-ZEC $(date +%Y-%m-%d_%H-%M-%S)\n##########################\n" >> ${LOGFILE}
${MINER_BINARY} -a equihash -o stratum+tcp://eu1-zcash.flypool.org:13333 -u ${wallet_address}.${worker_name} \
-p x -D -d ${device_id_cuda} --hash-log=${log_dir}/hash_log_ZEC_${device_id}.txt &>> ${LOGFILE}
