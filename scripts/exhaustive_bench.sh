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
	local CURRENCY=$1
	local OUTDIR=result_dir_${CURRENCY}
	mkdir $OUTDIR
	mkdir $OUTDIR/data
	mv result_${DEVICE_ID}* $OUTDIR
	mv log_${DEVICE_ID}_* power_results_${DEVICE_ID}_* $OUTDIR/data
}

function exhaustive_test {
	for CURRENCY in xmr eth zec;
	do
		save_call_command ./freq_exhaustive.exe ${CURRENCY} ${DEVICE_ID} 0 -1000 900 -200 200
		move_result_files $CURRENCY
		sleep 60
	done
}

pushd $BIN_DIR
exhaustive_test
popd

