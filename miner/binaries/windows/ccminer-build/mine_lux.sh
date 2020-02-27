#!/bin/bash

./ccminer -a phi2 -o stratum+tcp://phi2.mine.zpool.ca:8332 -u LUM85Kvt2Ex4sdCbLxJZUmWiP7akPHNTsW.lux_testworker -p c=LUX,mc=LUX -d $1 -D --hash-log=log.txt
