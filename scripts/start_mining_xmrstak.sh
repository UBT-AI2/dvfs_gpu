#!/bin/bash

user_data=$1
device_id=$2

../../miner/windows/xmr-stak/xmr-stak.exe --noCPU --nvidia nvidia$device_id.txt \
-O xmr-eu1.nanopool.org:14433 -u $user_data --currency monero7 -i 0 -p "" -r "" &
