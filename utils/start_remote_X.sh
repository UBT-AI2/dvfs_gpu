#!/bin/bash

#1) stop running x
if [ -x "$(command -v rcxdm)" ]; then
	#suse
	sudo rcxdm stop
elif [ -x "$(command -v lightdm)" ]; then
	#ubuntu 16.04
	sudo service lightdm stop
elif [ -x "$(command -v gdm3)" ]; then
	#ubuntu 18.04
	sudo service gdm3 stop
else
	echo 'Error: Unknown display manager. Stop X-Server manually.'
	exit 1
fi

#2) start new x on remote display
sudo nohup X -nolisten tcp :0 &

#3) set display environment variable
export DISPLAY=:0
