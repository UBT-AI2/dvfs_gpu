#!/bin/bash

device_id=$1
currency=$2
RESULT_DIR=$(date +%Y-%m-%d_%H-%M-%S)_result_gpu${device_id}_${currency}
############################################

mkdir ${RESULT_DIR}
mkdir ${RESULT_DIR}/data

mv result_${device_id}* ${RESULT_DIR}
mv log_${device_id}_* power_results_${device_id}_* ${RESULT_DIR}/data
