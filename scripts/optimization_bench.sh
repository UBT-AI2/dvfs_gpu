#!/bin/bash
BIN_DIR=../bin
DEVICE_ID=1

function save_call_command {
	"$@"
    local status=$?
    echo "Command '$@' exit code: $status"
    if [[ ${status} -ne 0 && ${status} -ne 127 ]]; then
        echo "Invalid exit code. Terminating script..."
		exit 1
    fi
}

function move_result_files {
	local METHOD=$1
	local CURRENCY=$2
	local OUTDIR=opt_result_${CURRENCY}_${METHOD}
	mkdir $OUTDIR
	mkdir $OUTDIR/data
	mv result_${DEVICE_ID}* $OUTDIR
	mv log_${DEVICE_ID}_* power_results_${DEVICE_ID}_* $OUTDIR/data
}

function optimization_test {
	for CURRENCY in eth zec xmr;
	do
		save_call_command ./freq_simulated_annealing.exe $CURRENCY $DEVICE_ID 6 300 10 -1000 900 -200 200
		move_result_files sa $CURRENCY
		sleep 60
		save_call_command ./freq_hill_climbing.exe $CURRENCY $DEVICE_ID 6 300 10 -1000 900 -200 200
		move_result_files hc $CURRENCY
		sleep 60
		save_call_command ./freq_nelder_mead.exe $CURRENCY $DEVICE_ID 8 300 10 -1000 900 -200 200
		move_result_files nm $CURRENCY
		sleep 60
	done
}

function optimization_min_hashrate_test {
	declare -A HASHRATE_MAP=( [eth]=34000000 [zec]=520 [xmr]=800 )
	for CURRENCY in eth zec xmr;
	do
		local min_hashrate=${HASHRATE_MAP[$CURRENCY]}
		save_call_command ./freq_simulated_annealing.exe $CURRENCY $DEVICE_ID 6 300 10 -1000 900 -200 200 $min_hashrate
		move_result_files sa_mh $CURRENCY
		sleep 60
		save_call_command ./freq_hill_climbing.exe $CURRENCY $DEVICE_ID 6 300 10 -1000 900 -200 200 $min_hashrate
		move_result_files hc_mh $CURRENCY
		sleep 60
		save_call_command ./freq_nelder_mead.exe $CURRENCY $DEVICE_ID 8 300 10 -1000 900 -200 200 $min_hashrate
		move_result_files nm_mh $CURRENCY
		sleep 60
	done
}

pushd $BIN_DIR
optimization_test
optimization_min_hashrate_test
popd

