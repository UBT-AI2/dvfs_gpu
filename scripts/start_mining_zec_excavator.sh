#!/bin/bash

device_id=$1
device_id_cuda=$2
wallet_address=$3
worker_name=$4
email=$5
log_dir=$6
pool_csv=$7
if [[ "$OSTYPE" == "msys" ]]
then
#MINGW
MINER_BINARY=../../miner/binaries/windows/excavator_v1.1.0a_Win64/excavator.exe
LOGFILE=${log_dir}/mining_log_excavator_gpu${device_id}.txt
else
#Linux
echo "exavator not available on linux"
exit 1
fi
#################################################################################

pool_list=($(echo ${pool_csv} | tr "," "\n"))
pool_option_str=${pool_list[0]}

echo -e "\n##########################\nSTARTED EXCAVATOR $(date +%Y-%m-%d_%H-%M-%S)\n##########################\n" >> ${LOGFILE}
${MINER_BINARY} -a equihash -s ${pool_option_str} -u ${wallet_address}:x/${worker_name}/${email} \
-p 0 -d 2 -cd $device_id_cuda 2>&1 | ./hash-log-excavator ${log_dir}/hash_log_ZEC_${device_id}.txt &>> ${LOGFILE}
