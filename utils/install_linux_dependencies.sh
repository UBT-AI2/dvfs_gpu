#!/bin/bash

if [ -x "$(command -v apt)" ]; then
	#debian
	sudo apt install libboost-all-dev libssl-dev libgoogle-glog-dev libeigen3-dev libcurl4-openssl-dev \
	libjansson-dev libmicrohttpd-dev libhwloc-dev libx11-dev automake autotools-dev build-essential cmake
elif [ -x "$(command -v zypper)" ]; then
	#suse
	sudo zypper in boost-devel libopenssl-devel glog-devel eigen3-devel libcurl-devel \
	libjansson-devel libmicrohttpd-devel hwloc-devel libX11-devel automake cmake
else
	echo 'Error: Unknown package manager. Only apt (debian) and zypper (suse) supported.'
	exit 1
fi


