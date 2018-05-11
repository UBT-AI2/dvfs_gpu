#!/bin/bash
kill $(ps -e | grep gpu_power_monitor | awk 'NR==1 {print $1}')