#!/bin/bash

./ccminer -a bitcore -o stratum+tcp://bitcore.mine.zpool.ca:3556 -u 2QjLfa8gmcF5WDjYgeQUi2EbMtn5oE3UNj.btx_testworker -p c=BTX,mc=BTX -d $1 -D --hash-log=log.txt
