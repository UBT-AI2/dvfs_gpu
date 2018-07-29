#!/bin/bash
BIN_DIR=../bin
DEVICE_ID=0

function save_call_command {
	"$@"
    local status=$?
    echo "Command '$@' exit code: $status"
    if [[ ${status} -ne 0 && ${status} -ne 127 ]]; then
        echo "Invalid exit code. Terminating script..."
		exit 1
    fi
}

function exhaustive_test {
	for CURRENCY in eth zec;
	do
		save_call_command ./freq_exhaustive.exe ${CURRENCY} ${DEVICE_ID}
		sleep 60
	done
}

pushd $BIN_DIR
exhaustive_test
popd

