#!/bin/bash

POWERFILE=${log_dir}/power_results_${device_id}.txt
if [[ "$OSTYPE" == "msys" ]]
then
#MINGW
MINER_BINARY=../../miner/binaries/windows/ccminer-build/ccminer.exe
else
#Linux
MINER_BINARY=../../miner/binaries/linux/ccminer-build/ccminer
fi
######################################################################################################

function ccminer_run_benchmark {
    local currency=$1
    local algo=$2
    local RESULTFILE=${log_dir}/offline_bench_result_gpu${device_id}_${currency}.dat

    bench_start=$(tail -n 1 ${POWERFILE} | awk '{print $1}')
    time_start=$(($(date +%s%N)/1000000))
    BENCH_LOGCMD="$(${MINER_BINARY} -a ${algo} --benchmark -d ${device_id_cuda} 2>&1)"
    time_dur=$(($(($(date +%s%N)/1000000)) - ${time_start}))

    if [[ -z $bench_start ]]
    then
        BENCH_POWERCMD="$(cat ${POWERFILE})"
    else
        BENCH_POWERCMD="$(grep -A1000 "${bench_start}" ${POWERFILE})"
    fi

    max_power=$(echo "${BENCH_POWERCMD}" | awk 'BEGIN{a=0}{if ($2>0+a) a=$2} END{print a}')
    avg_hashrate=$(echo "${BENCH_LOGCMD}" | grep -oP "${algo} hashrate =\s*\K[+-]?[0-9]+([.][0-9]+)?")
    hashes_per_joule=$(awk "BEGIN { print ${avg_hashrate} / ${max_power} }")

    echo ${mem_clock},${graph_clock},${max_power},${avg_hashrate},${hashes_per_joule},${time_dur},${time_start} >> ${RESULTFILE}
}


function ccminer_start_mining {
    local currency=$1
    local algo=$2
    local pool_address=$3
    local LOGFILE=${log_dir}/mining_log_ccminer-${currency}_gpu${device_id}.txt

    echo -e "\n##########################\nSTARTED CCMINER-${currency} $(date +%Y-%m-%d_%H-%M-%S)\n##########################\n" >> ${LOGFILE}
    ${MINER_BINARY} -a ${algo} -o stratum+tcp://${pool_address} -u ${wallet_address}.${worker_name} \
    -p x -D -d ${device_id_cuda} --hash-log=${log_dir}/hash_log_${currency}_${device_id}.txt &>> ${LOGFILE}
}