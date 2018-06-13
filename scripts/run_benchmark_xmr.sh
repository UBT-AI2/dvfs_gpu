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
MINER_BINARY=../../miner/binaries/windows/xmr-stak-build/xmr-stak.exe
else
#Linux
MINER_BINARY=../../miner/binaries/linux/xmr-stak-build/xmr-stak
fi
#################################################################################

bench_start=$(tail -n 1 ${POWERFILE} | awk '{print $1}')
${MINER_BINARY} --noCPU --benchmark 6 --nvidia nvidia$device_id.txt &> ${BENCH_LOGFILE}

if [[ -z $bench_start ]]
then
	cat ${POWERFILE} > ${BENCH_POWERFILE}
else
	grep -A1000 "${bench_start}" ${POWERFILE} > ${BENCH_POWERFILE}
fi

max_power=$(awk 'BEGIN{a=0}{if ($2>0+a) a=$2} END{print a}' ${BENCH_POWERFILE})
avg_hashrate=$(grep -i 'Benchmark Total:' ${BENCH_LOGFILE} | grep -Eo '[+-]?[0-9]+([.][0-9]+)?' | tail -n 1)
hashes_per_joule=$(awk "BEGIN { print ${avg_hashrate} / ${max_power} }")

echo ${mem_clock},${graph_clock},${max_power},${avg_hashrate},${hashes_per_joule} >> result_${device_id}.dat

