#!/bin/bash

device_id=$1
device_id_cuda=$2
wallet_address=$3
worker_name=$4
email=$5
log_dir=$6
pool_csv=$7
pool_pass=$8
currency_name=$9
algo=$10
#################################################################################

source $(dirname $(readlink -f $0))/ccminer_generic.sh
ccminer_start_mining ${currency_name} ${algo}