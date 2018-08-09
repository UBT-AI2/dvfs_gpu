#!/bin/bash

device_id=$1
device_id_cuda=$2
mem_clock=$3
graph_clock=$4
log_dir=$5
POWERFILE=${log_dir}/power_results_${device_id}.txt
if [[ "$OSTYPE" == "msys" ]]
then
#MINGW
MINER_BINARY=../../miner/binaries/windows/ccminer-build/ccminer.exe
else
#Linux
MINER_BINARY=../../miner/binaries/linux/ccminer-build/ccminer
fi
#################################################################################

bench_start=$(tail -n 1 ${POWERFILE} | awk '{print $1}')
time_start=$(($(date +%s%N)/1000000))
BENCH_LOGCMD="$(${MINER_BINARY} -a equihash --benchmark -d ${device_id_cuda} 2>&1)"
time_dur=$(($(($(date +%s%N)/1000000)) - ${time_start}))

if [[ -z $bench_start ]]
then
	BENCH_POWERCMD="$(cat ${POWERFILE})"
else
	BENCH_POWERCMD="$(grep -A1000 "${bench_start}" ${POWERFILE})"
fi

max_power=$(echo "${BENCH_POWERCMD}" | awk 'BEGIN{a=0}{if ($2>0+a) a=$2} END{print a}')
avg_hashrate=$(echo "${BENCH_LOGCMD}" | grep -oP 'equihash hashrate =\s*\K[+-]?[0-9]+([.][0-9]+)?')
hashes_per_joule=$(awk "BEGIN { print ${avg_hashrate} / ${max_power} }")

echo ${mem_clock},${graph_clock},${max_power},${avg_hashrate},${hashes_per_joule},${time_dur},${time_start} \
>> ${log_dir}/offline_bench_result_gpu${device_id}_XMR.dat
