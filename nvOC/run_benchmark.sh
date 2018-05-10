#!/bin/bash

device_id=$1
mem_clock=$2
graph_clock=$3

#../../miner/windows/ethminer-0.9.41-genoil-1.1.7/ethminer.exe -U -M --cuda-devices $device_id > log_${mem_clock}_${graph_clock}.txt
../../miner/windows/excavator_v1.1.0a_Win64/excavator.exe -b -a equihash -cd $device_id &> log_${mem_clock}_${graph_clock}.txt
avg_hashrate=$(grep -i 'Total measured:' log_${mem_clock}_${graph_clock}.txt | grep -Eo '[+-]?[0-9]+([.][0-9]+)?' | tail -n 1)

cat power_results.txt | tail -n 50 > power_results_${mem_clock}_${graph_clock}.txt

#bench_time=$(bc -l <<< "$(awk 'END {print $1}' power_results_${mem_clock}_${graph_clock}.txt)-$(awk 'NR==1 {print $1}' power_results_${mem_clock}_${graph_clock}.txt)")
avg_power=$(awk '{ total += $2 } END { print total/NR }' power_results_${mem_clock}_${graph_clock}.txt)
#t1=$(grep 'Trial 1...' log_${mem_clock}_${graph_clock}.txt | sed 's/Trial [0-9]... //g')
#t2=$(grep 'Trial 2...' log_${mem_clock}_${graph_clock}.txt | sed 's/Trial [0-9]... //g')
#t3=$(grep 'Trial 3...' log_${mem_clock}_${graph_clock}.txt | sed 's/Trial [0-9]... //g')
#t4=$(grep 'Trial 4...' log_${mem_clock}_${graph_clock}.txt | sed 's/Trial [0-9]... //g')
#t5=$(grep 'Trial 5...' log_${mem_clock}_${graph_clock}.txt | sed 's/Trial [0-9]... //g')
#avg_hashrate=$(./calc_inner_mean $t1 $t2 $t3 $t4 $t5)
#hashes_per_joule=$(bc -l <<< "${avg_hashrate}/${avg_power}")

#echo ${mem_clock},${graph_clock},${bench_time},${avg_power},${avg_hashrate},${hashes_per_joule} >> result.dat
echo ${mem_clock},${graph_clock},${avg_power},${avg_hashrate} >> result.dat


