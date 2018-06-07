#!/bin/bash
BIN_DIR=../bin
DEVICE_ID=1

function move_result_files{
	local METHOD=$1
	local CURRENCY=$2
	local OUTDIR=result_dir_$METHOD_$CURRENCY
	mkdir $OUTDIR
	mkdir $OUTDIR/data
	mv result_$DEVICE_ID* $OUTDIR
	mv log_$DEVICE_ID* power_results_$DEVICE_ID* $OUTDIR/data
}

function optimization_test {
	for CURRENCY in eth zec xmr;
	do
		./freq_simulated_annealing.exe $CURRENCY $DEVICE_ID 6 300 10 -1000 950
		move_result_files sa $CURRENCY
		./freq_hill_climbing.exe $CURRENCY $DEVICE_ID 6 300 10 -1000 950
		move_result_files hc $CURRENCY
		./freq_nelder_mead.exe $CURRENCY $DEVICE_ID 8 300 10 -1000 950
		move_result_files nm $CURRENCY
	done
}

function optimization_min_hashrate_test {
	declare -A HASHRATE_MAP=( [eth]=34000000 [zec]=520 [xmr]=800 )
	for CURRENCY in eth zec xmr;
	do
		local min_hashrate=${HASHRATE_MAP[$CURRENCY]}
		./freq_simulated_annealing.exe $CURRENCY $DEVICE_ID 6 300 10 -1000 950 $min_hashrate
		move_result_files sa_mh $CURRENCY
		./freq_hill_climbing.exe $CURRENCY $DEVICE_ID 6 300 10 -1000 950 $min_hashrate
		move_result_files hc_mh $CURRENCY
		./freq_nelder_mead.exe $CURRENCY $DEVICE_ID 8 300 10 -1000 950 $min_hashrate
		move_result_files nm_mh $CURRENCY
	done
}

pushd $BIN_DIR
optimization_test
optimization_min_hashrate_test
popd

