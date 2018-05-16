#!/bin/bash

name=$1

kill $(ps -e | grep $name | awk 'NR==1 {print $1}')
