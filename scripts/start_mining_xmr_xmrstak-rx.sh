#!/bin/bash

device_id=$1
device_id_cuda=$2
wallet_address=$3
worker_name=$4
email=$5
log_dir=$(readlink -f $6)
pool_csv=$7
pool_pass=$8
if [[ "$OSTYPE" == "msys" ]]
then
#MINGW
MINER_BINARY=./miner/binaries/windows/xmr-stak-rx-build/xmr-stak-rx.exe
else
#Linux
MINER_BINARY=./miner/binaries/linux/xmr-stak-rx-build/xmr-stak-rx
fi
MINER_BINARY=$(readlink -f ${MINER_BINARY})
LOGFILE=${log_dir}/mining_log_xmr-stak-rx_gpu${device_id}.txt
#################################################################################

pool_list=($(echo ${pool_csv} | tr "," "\n"))
pool_option_str=${pool_list[0]}

echo -e "\n##########################\nSTARTED XMR-STAK-RX $(date +%Y-%m-%d_%H-%M-%S)\n##########################\n" >> ${LOGFILE}
pushd $(dirname ${MINER_BINARY}) > /dev/null
${MINER_BINARY} --noCPU --noTest --cuda-devices ${device_id_cuda} \
-o ${pool_option_str} -p ${pool_pass} -u ${wallet_address}.${worker_name}/${email} \
 --currency monero -i 0 -r "" --hash-logfile ${log_dir}/hash_log_XMR_${device_id}.txt &>> ${LOGFILE}
popd > /dev/null
