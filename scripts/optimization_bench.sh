#!/bin/bash
BIN_DIR=../bin
CONFIG_JSON=../configs/currency_config_default.json

function save_call_command {
	"$@"
    local status=$?
    echo "Command '$@' exit code: $status"
    if [[ ${status} -ne 0 && ${status} -ne 127 ]]; then
        echo "Invalid exit code. Terminating script..."
		exit 1
    fi
}

function optimization_test {
	local DEVICE_ID=$1
	for CURRENCY in XMR ETH ZEC VTC RVN BTX LUX;
	do
		save_call_command ./freq_simulated_annealing.exe $CONFIG_JSON $CURRENCY $DEVICE_ID 5 0.15 0.15
		mv offline_bench_result_gpu${DEVICE_ID}_${CURRENCY}.dat offline_bench_result_gpu${DEVICE_ID}_${CURRENCY}_SA.dat
		sleep 60
		save_call_command ./freq_hill_climbing.exe $CONFIG_JSON $CURRENCY $DEVICE_ID 5 0.15 0.15
		mv offline_bench_result_gpu${DEVICE_ID}_${CURRENCY}.dat offline_bench_result_gpu${DEVICE_ID}_${CURRENCY}_HC.dat
		sleep 60
		save_call_command ./freq_nelder_mead.exe $CONFIG_JSON $CURRENCY $DEVICE_ID 8 0.15 0.15
		mv offline_bench_result_gpu${DEVICE_ID}_${CURRENCY}.dat offline_bench_result_gpu${DEVICE_ID}_${CURRENCY}_NM.dat
		sleep 60
	done
}

function optimization_min_hashrate_test {
	local DEVICE_ID=$1
	local min_hashrate=$2
	for CURRENCY in XMR ETH ZEC VTC RVN BTX LUX;
	do
		save_call_command ./freq_simulated_annealing.exe $CONFIG_JSON $CURRENCY $DEVICE_ID 5 0.15 0.15 $min_hashrate
		mv offline_bench_result_gpu${DEVICE_ID}_${CURRENCY}.dat offline_bench_result_gpu${DEVICE_ID}_${CURRENCY}_SA_mh.dat
		sleep 60
		save_call_command ./freq_hill_climbing.exe $CONFIG_JSON $CURRENCY $DEVICE_ID 5 0.15 0.15 $min_hashrate
		mv offline_bench_result_gpu${DEVICE_ID}_${CURRENCY}.dat offline_bench_result_gpu${DEVICE_ID}_${CURRENCY}_HC_mh.dat
		sleep 60
		save_call_command ./freq_nelder_mead.exe $CONFIG_JSON $CURRENCY $DEVICE_ID 8 0.15 0.15 $min_hashrate
		mv offline_bench_result_gpu${DEVICE_ID}_${CURRENCY}.dat offline_bench_result_gpu${DEVICE_ID}_${CURRENCY}_NM_mh.dat
		sleep 60
	done
}

function exhaustive_test {
	local DEVICE_ID=$1
	for CURRENCY in LUX;
	do
		save_call_command ./freq_exhaustive.exe $CONFIG_JSON $CURRENCY $DEVICE_ID
		mv offline_bench_result_gpu${DEVICE_ID}_${CURRENCY}.dat exhaustive_bench_result_gpu${DEVICE_ID}_${CURRENCY}.dat
		sleep 60
	done
}

pushd $BIN_DIR
for id in 0;
do
	optimization_test $id
	optimization_min_hashrate_test $id 0.85
done
	#exhaustive_test 1
popd

