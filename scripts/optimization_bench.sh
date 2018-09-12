#!/bin/bash
BIN_DIR=../bin
USER_CONFIG=../configs/titanX_2x1080_p4000_user_config.json

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
		for OPT_ALGO in HC SA NM;
		do
            save_call_command ./freq_optimization.exe -d $DEVICE_ID -c $CURRENCY -a $OPT_ALGO
            mv offline_bench_result_gpu${DEVICE_ID}_${CURRENCY}.dat offline_bench_result_gpu${DEVICE_ID}_${CURRENCY}_${OPT_ALGO}.dat
            sleep 60
		done
	done
}

function optimization_min_hashrate_test {
	local DEVICE_ID=$1
	local min_hashrate=$2
	for CURRENCY in XMR ETH ZEC VTC RVN BTX LUX;
	do
		for OPT_ALGO in HC SA NM;
		do
            save_call_command ./freq_optimization.exe -d $DEVICE_ID -c $CURRENCY -a $OPT_ALGO --min_hashrate=$min_hashrate
            mv offline_bench_result_gpu${DEVICE_ID}_${CURRENCY}.dat offline_bench_result_gpu${DEVICE_ID}_${CURRENCY}_${OPT_ALGO}_mh.dat
            sleep 60
		done
	done
}

function exhaustive_test {
	local DEVICE_ID=$1
	for CURRENCY in VTC RVN BTX;
	do
		save_call_command ./freq_exhaustive.exe -d $DEVICE_ID -c $CURRENCY
		mv offline_bench_result_gpu${DEVICE_ID}_${CURRENCY}.dat exhaustive_bench_result_gpu${DEVICE_ID}_${CURRENCY}.dat
		sleep 60
	done
}

pushd $BIN_DIR
#for id in 0;
#do
	#optimization_test $id
	#optimization_min_hashrate_test $id 0.85
#done
	exhaustive_test 0
	exhaustive_test 2
popd

