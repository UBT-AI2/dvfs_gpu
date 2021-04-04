#!/bin/bash

device_id=$1
device_id_cuda=$2
mem_clock=$3
graph_clock=$4
log_dir=$(readlink -f $5)
currency_name=$6
algo=$7
#################################################################################

source ./scripts/ccminer_generic.sh
ccminer_run_benchmark ${currency_name} ${algo}