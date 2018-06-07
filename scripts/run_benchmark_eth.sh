#!/bin/bash

device_id=$1
device_id_cuda=$2
mem_clock=$3
graph_clock=$4
POWERFILE=power_results_${device_id}.txt
BENCH_POWERFILE=power_results_${device_id}_${mem_clock}_${graph_clock}.txt
BENCH_LOGFILE=log_${device_id}_${mem_clock}_${graph_clock}.txt
if [[ "$OSTYPE" == "msys" ]]
then
#MINGW
MINER_BINARY=../../miner/binaries/windows/ethminer-build/ethminer.exe
else
#Linux
MINER_BINARY=../../miner/binaries/linux/ethminer-build/ethminer
fi
#################################################################################

bench_start=$(tail -n 1 ${POWERFILE} | awk '{print $1}')
${MINER_BINARY} -U -M \
--benchmark-trials 1 --benchmark-warmup 10 --cuda-devices $device_id_cuda &> ${BENCH_LOGFILE}

if [[ -z $bench_start ]]
then
	cat ${POWERFILE} > ${BENCH_POWERFILE}
else
	grep -A1000 "${bench_start}" ${POWERFILE} > ${BENCH_POWERFILE}
fi

max_power=$(awk 'BEGIN{a=0}{if ($2>0+a) a=$2} END{print a}' ${BENCH_POWERFILE})
avg_hashrate=$(grep -A1 '^Trial 1...' ${BENCH_LOGFILE} | tail -n1)
hashes_per_joule=$(awk "BEGIN { print ${avg_hashrate} / ${max_power} }")

echo ${mem_clock},${graph_clock},${max_power},${avg_hashrate},${hashes_per_joule} >> result_${device_id}.dat


