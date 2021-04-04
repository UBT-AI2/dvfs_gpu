#!/bin/bash
POWERFILE=${log_dir}/power_results_${device_id}.txt

function generic_bench {
    currency=$1
    bench_cmd=$2
    hashrate_extraction_cmd=$3
    RESULTFILE=${log_dir}/offline_bench_result_gpu${device_id}_${currency}.dat
    MINER_OUTPUT_FILE=${log_dir}/miner_output_offline_bench_gpu${device_id}_${currency}.txt

    echo -e "\n##########################\nSTARTING BENCHMARK-${currency} with mem_clock=${mem_clock}, graph_clock=${graph_clock} [$(date +%Y-%m-%d_%H-%M-%S)]\n##########################\n" >> ${MINER_OUTPUT_FILE}

    bench_start=$(tail -n 1 ${POWERFILE} | awk '{print $1}')
    pushd $(dirname ${MINER_BINARY}) > /dev/null
    time_start=$(($(date +%s%N)/1000000))
    #warmuptime strongly affects performance
    BENCH_LOGCMD="$(eval ${bench_cmd} 2>&1 | tee -a ${MINER_OUTPUT_FILE})"
    time_dur=$(($(($(date +%s%N)/1000000)) - ${time_start}))
    popd > /dev/null

    if [[ -z $bench_start ]]
    then
        BENCH_POWERCMD="$(cat ${POWERFILE})"
    else
        BENCH_POWERCMD="$(grep -A1000 "${bench_start}" ${POWERFILE})"
    fi

    max_power=$(echo "${BENCH_POWERCMD}" | awk 'BEGIN{a=0}{if ($2>0+a) a=$2} END{print a}')
    #avg_power=$(echo "${BENCH_POWERCMD}" | awk '{ if (NR > 0) {sum += $2; n++} } END { if (n > 0) {print sum / n} else 0 }')
    avg_hashrate=$(echo "${BENCH_LOGCMD}" | eval LC_ALL=en_US.utf8 ${hashrate_extraction_cmd})
    hashes_per_joule=$(awk "BEGIN { print ${avg_hashrate} / ${max_power} }")

    echo ${mem_clock},${graph_clock},${max_power},${avg_hashrate},${hashes_per_joule},${time_dur},${time_start} >> ${RESULTFILE}
}