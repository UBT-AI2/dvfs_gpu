#!/bin/bash

./ethminer --farm-recheck 2000 -U -RH \
-P stratum1+tcp://0x8291ca623a1f1a877fa189b594f6098c74aad0b3:x@eth-eu2.nanopool.org:9999/testworker_eth/alexander.fiebig@uni-bayreuth.de \
-P stratum1+tcp://0x8291ca623a1f1a877fa189b594f6098c74aad0b3:x@eth-eu1.nanopool.org:9999/testworker_eth/alexander.fiebig@uni-bayreuth.de \
--cuda-devices $1 --cuda-parallel-hash 8 
#--mining-duration 60 --hash-logfile hash-log-eth.txt