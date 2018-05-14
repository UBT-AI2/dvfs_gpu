#!/bin/bash

device_id=$1
mem_clock=$2
graph_clock=$3

../../miner/windows/excavator_v1.1.0a_Win64/excavator.exe -b -a equihash -cd $device_id &> log_${mem_clock}_${graph_clock}.txt

cat power_results.txt | tail -n 11 > power_results_${mem_clock}_${graph_clock}.txt
#bench_time=$(bc -l <<< "$(awk 'END {print $1}' power_results_${mem_clock}_${graph_clock}.txt)-$(awk 'NR==1 {print $1}' power_results_${mem_clock}_${graph_clock}.txt)")
avg_power=$(awk 'NR<11{ total += $2 } END { print total/10 }' power_results_${mem_clock}_${graph_clock}.txt)

avg_hashrate=$(grep -i 'Total measured:' log_${mem_clock}_${graph_clock}.txt | grep -Eo '[+-]?[0-9]+([.][0-9]+)?' | tail -n 1)
hashes_per_joule=$(awk "BEGIN { print ${avg_hashrate} / ${avg_power} }")

echo ${mem_clock},${graph_clock},${avg_power},${avg_hashrate},${hashes_per_joule} >> result.dat


