#!/bin/bash

if [[ "$OSTYPE" == "msys" ]]
then
#MINGW
MINER_BINARY=./miner/binaries/windows/ccminer-build/ccminer.exe
else
#Linux
MINER_BINARY=./miner/binaries/linux/ccminer-build/ccminer
fi
MINER_BINARY=$(readlink -f ${MINER_BINARY})
######################################################################################################
source ./scripts/util_functions.sh

function ccminer_run_benchmark {
    currency=$1
    algo=$2

    generic_bench ${currency} \
    '${MINER_BINARY} -a ${algo} --benchmark -d ${device_id_cuda}' \
    'grep -oP "${algo} hashrate =\s*\K[+-]?[0-9]+([.][0-9]+)?"'
}


function ccminer_start_mining {
    currency=$1
    algo=$2
    LOGFILE=${log_dir}/mining_log_ccminer-${currency}_gpu${device_id}.txt

    pool_list=($(echo ${pool_csv} | tr "," "\n"))
    for pool in "${pool_list[@]}"
    do
        pool_option_str="${pool_option_str} -a ${algo} -o stratum+tcp://${pool} -u ${wallet_address}.${worker_name} -p ${pool_pass}"
    done

    echo -e "\n##########################\nSTARTED CCMINER-${currency} $(date +%Y-%m-%d_%H-%M-%S)\n##########################\n" >> ${LOGFILE}
    pushd $(dirname ${MINER_BINARY}) > /dev/null
    ${MINER_BINARY} ${pool_option_str} \
    -D -d ${device_id_cuda} --hash-log=${log_dir}/hash_log_${currency}_${device_id}.txt &>> ${LOGFILE}
    popd > /dev/null
}