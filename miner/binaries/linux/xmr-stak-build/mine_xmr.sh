#!/bin/bash

./xmr-stak --noCPU --cuda-devices $1 -o xmr-eu2.nanopool.org:14444 -p x \
-u 49obKYMTctj2owFCjjPwmDELGNCc7kz3WBVLGgpF1MC3cWYH3psdpyV8rBdZUycYPr3qU9ChEmj4ZMFLLf2gN2bcFEzNPpv.testworker_xmr/alexander.fiebig@uni-bayreuth.de \
--currency monero -i 0 -r "" --mining-duration 60 --hash-logfile hash-log-xmr.txt
