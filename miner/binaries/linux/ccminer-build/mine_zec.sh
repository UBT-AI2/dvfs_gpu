#!/bin/bash

WALLET_ADRESS=t1gRqCMixZffv3qtyBpQ1fQ5wd93ayuyTKx.$2

#Because the stratum implementation varies on different pools, a miner may have connection problems with pools other than YIIMP when mining Equihash algorithm with CCminer.  
#For a more detailed answer, you will need to ask Epsylon3.  Basically, I would say that it is a "Work in Progress".

#zec-eu1.nanopool.org:6666
./ccminer -a equihash -o stratum+tcp://eu1-zcash.flypool.org:13333 -u $WALLET_ADRESS -p x -d $1 -D --hash-log=log.txt