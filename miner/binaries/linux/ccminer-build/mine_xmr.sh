#!/bin/bash

WALLET_ADRESS=49obKYMTctj2owFCjjPwmDELGNCc7kz3WBVLGgpF1MC3cWYH3psdpyV8rBdZUycYPr3qU9ChEmj4ZMFLLf2gN2bcFEzNPpv.worker_pascal/alexander.fiebig@uni-bayreuth.de

./ccminer -a cryptonight -o stratum+tcp://xmr-eu1.nanopool.org:14444 -u $WALLET_ADRESS -p x -d 0