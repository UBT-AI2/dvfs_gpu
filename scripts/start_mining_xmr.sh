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
MINER_BINARY=../../miner/binaries/windows/xmr-stak-build/xmr-stak.exe
else
#Linux
MINER_BINARY=../../miner/binaries/linux/xmr-stak-build/xmr-stak
fi
LOGFILE=${log_dir}/mining_log_xmr-stak_gpu${device_id}.txt
#################################################################################

echo -e "\n##########################\nSTARTED XMR-STAK $(date +%Y-%m-%d_%H-%M-%S)\n##########################\n" >> ${LOGFILE}
${MINER_BINARY} --noCPU --cuda-devices ${device_id_cuda} \
-O xmr-eu2.nanopool.org:14433 -u ${wallet_address}.${worker_name}/${email} \
 --currency monero7 -i 0 -p "" -r "" --hash-logfile ${log_dir}/hash_log_XMR_${device_id}.txt &>> ${LOGFILE}
