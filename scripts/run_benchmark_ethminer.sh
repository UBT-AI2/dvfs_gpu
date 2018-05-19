#!/bin/bash

device_id=$1
mem_clock=$2
graph_clock=$3

bench_start=$(tail -n 1 power_results.txt | awk '{print $1}')

../../miner/windows/ethminer-0.9.41-genoil-1.1.7/ethminer.exe -U -M --cuda-devices $device_id > log_${mem_clock}_${graph_clock}.txt

if [[ -z $bench_start ]]
then
	cat power_results.txt > power_results_${mem_clock}_${graph_clock}.txt
else
	grep -A1000 "${bench_start}" power_results.txt > power_results_${mem_clock}_${graph_clock}.txt
fi

#bench_time=$(bc -l <<< "$(awk 'END {print $1}' power_results_${mem_clock}_${graph_clock}.txt)-$(awk 'NR==1 {print $1}' power_results_${mem_clock}_${graph_clock}.txt)")
#avg_power=$(awk '{ total += $2 } END { print total/NR }' power_results_${mem_clock}_${graph_clock}.txt)

max_power=$(awk 'BEGIN{a=0}{if ($2>0+a) a=$2} END{print a}' power_results_${mem_clock}_${graph_clock}.txt)

t1=$(grep 'Trial 1...' log_${mem_clock}_${graph_clock}.txt | sed 's/Trial [0-9]... //g')
t2=$(grep 'Trial 2...' log_${mem_clock}_${graph_clock}.txt | sed 's/Trial [0-9]... //g')
t3=$(grep 'Trial 3...' log_${mem_clock}_${graph_clock}.txt | sed 's/Trial [0-9]... //g')
t4=$(grep 'Trial 4...' log_${mem_clock}_${graph_clock}.txt | sed 's/Trial [0-9]... //g')
t5=$(grep 'Trial 5...' log_${mem_clock}_${graph_clock}.txt | sed 's/Trial [0-9]... //g')
avg_hashrate=$(./calc_inner_mean $t1 $t2 $t3 $t4 $t5)
hashes_per_joule=$(awk "BEGIN { print ${avg_hashrate} / ${max_power} }")

echo ${mem_clock},${graph_clock},${max_power},${avg_hashrate},${hashes_per_joule} >> result.dat


