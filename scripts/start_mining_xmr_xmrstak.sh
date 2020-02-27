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
MINER_BINARY=./miner/binaries/windows/xmr-stak-build/xmr-stak.exe
else
#Linux
MINER_BINARY=./miner/binaries/linux/xmr-stak-build/xmr-stak
fi
LOGFILE=${log_dir}/mining_log_xmr-stak_gpu${device_id}.txt
#################################################################################

pool_list=($(echo ${pool_csv} | tr "," "\n"))
pool_option_str=${pool_list[0]}

echo -e "\n##########################\nSTARTED XMR-STAK $(date +%Y-%m-%d_%H-%M-%S)\n##########################\n" >> ${LOGFILE}
${MINER_BINARY} --noCPU --cuda-devices ${device_id_cuda} \
-o ${pool_option_str} -p ${pool_pass} -u ${wallet_address}.${worker_name}/${email} \
 --currency cryptonight_v8 -i 0 -r "" --hash-logfile ${log_dir}/hash_log_XMR_${device_id}.txt &>> ${LOGFILE}
