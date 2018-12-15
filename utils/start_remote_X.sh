#!/bin/bash

#1)stop running x
#suse
#sudo rcxdm stop
#ubuntu
sudo service lightdm stop

#start new x on remote display
sudo nohup X -nolisten tcp :0 &
