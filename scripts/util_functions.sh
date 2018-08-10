#!/bin/bash
POWERFILE=${log_dir}/power_results_${device_id}.txt

function generic_bench {
    currency=$1
    bench_cmd=$2
    hashrate_extraction_cmd=$3
    RESULTFILE=${log_dir}/offline_bench_result_gpu${device_id}_${currency}.dat

    bench_start=$(tail -n 1 ${POWERFILE} | awk '{print $1}')
    time_start=$(($(date +%s%N)/1000000))
    #warmuptime strongly affects performance
    BENCH_LOGCMD="$(eval ${bench_cmd} 2>&1)"
    time_dur=$(($(($(date +%s%N)/1000000)) - ${time_start}))

    if [[ -z $bench_start ]]
    then
        BENCH_POWERCMD="$(cat ${POWERFILE})"
    else
        BENCH_POWERCMD="$(grep -A1000 "${bench_start}" ${POWERFILE})"
    fi

    max_power=$(echo "${BENCH_POWERCMD}" | awk 'BEGIN{a=0}{if ($2>0+a) a=$2} END{print a}')
    avg_hashrate=$(echo "${BENCH_LOGCMD}" | eval ${hashrate_extraction_cmd})
    hashes_per_joule=$(awk "BEGIN { print ${avg_hashrate} / ${max_power} }")

    echo ${mem_clock},${graph_clock},${max_power},${avg_hashrate},${hashes_per_joule},${time_dur},${time_start} >> ${RESULTFILE}
}