#!/bin/bash

user_data=$1
device_id=$2

../../miner/windows/excavator_v1.1.0a_Win64/excavator.exe -a equihash -s zec-eu1.nanopool.org:6666 -u $user_data -p 0 -d 2 -cd $device_id &
