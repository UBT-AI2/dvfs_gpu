#!/bin/bash

./xmr-stak --noCPU --currency monero --benchmark 8 --benchwait 5 --benchwork 10 --cuda-devices $1
