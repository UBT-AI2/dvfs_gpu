#!/bin/bash

user_data=$1
device_id=$2

../../miner/windows/ethminer-0.9.41-genoil-1.1.7/ethminer.exe --farm-recheck 200 -U -S eth-eu1.nanopool.org:9999 -FS eth-eu2.nanopool.org:9999 \
-O $user_data --cuda-devices $device_id --cuda-parallel-hash 8 &

