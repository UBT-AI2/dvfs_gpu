#!/bin/bash

device_id=$1
device_id_cuda=$2
mem_clock=$3
graph_clock=$4
log_dir=$5
#################################################################################

source $(dirname $(readlink -f $0))/ccminer_generic.sh
ccminer_run_benchmark ZEC equihash