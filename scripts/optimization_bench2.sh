#!/bin/bash
BIN_DIR=../bin
CURRENCY_CONFIG=../configs/currency_config_default.json
declare -A WALLET_MAP
WALLET_MAP[ETH]=0x8291ca623a1f1a877fa189b594f6098c74aad0b3
WALLET_MAP[ZEC]=t1Z8gLLGyxGRkjRFbNnJ2n6yvHb1Vo3pXKH
WALLET_MAP[XMR]=49obKYMTctj2owFCjjPwmDELGNCc7kz3WBVLGgpF1MC3cWYH3psdpyV8rBdZUycYPr3qU9ChEmj4ZMFLLf2gN2bcFEzNPpv
WALLET_MAP[LUX]=LUM85Kvt2Ex4sdCbLxJZUmWiP7akPHNTsW
WALLET_MAP[BTX]=2QjLfa8gmcF5WDjYgeQUi2EbMtn5oE3UNj
WALLET_MAP[VTC]=VpGMpfWuQxR4rWJp3i8Qc7wqwf4yLQtdLd
WALLET_MAP[RVN]=RKNJZyAAzbPT3Nvrujzp2m5HLB7eA1MZZS
#
gpu=$1
mem_step=$2
graph_step=$3
max_mem_oc=$4
max_graph_oc=$5

function save_call_command {
	"$@"
    local status=$?
    echo "Command '$@' exit code: $status"
    if [[ status -ne 0 && status -ne 127 ]];
	then
        echo "Invalid exit code. Terminating script..."
		exit 1
    fi
}

function optimization_test {
	local DEVICE_ID=$1
	local online=$2
	for CURRENCY in ZEC;
	do
		for OPT_ALGO in HC;
		do
			if [[ online -eq 0 ]];
			then
				#save_call_command ./freq_optimization -d $DEVICE_ID -c $CURRENCY -a $OPT_ALGO \
				#	--start_mem_oc=900 --start_graph_idx=0 --max_iterations=6 --currency_config=$CURRENCY_CONFIG
				#mv offline_bench_result_gpu${DEVICE_ID}_${CURRENCY}.dat opt_bench_result_gpu${DEVICE_ID}_${CURRENCY}_${OPT_ALGO}_max_offline.dat
				#save_call_command ./freq_optimization -d $DEVICE_ID -c $CURRENCY -a $OPT_ALGO \
				#	--start_mem_oc=-50 --start_graph_idx=10 --max_iterations=6 --currency_config=$CURRENCY_CONFIG
				#mv offline_bench_result_gpu${DEVICE_ID}_${CURRENCY}.dat opt_bench_result_gpu${DEVICE_ID}_${CURRENCY}_${OPT_ALGO}_mid_offline.dat
				#save_call_command ./freq_optimization -d $DEVICE_ID -c $CURRENCY -a $OPT_ALGO \
				#	--start_mem_oc=-1000 --start_graph_idx=21 --max_iterations=6 --currency_config=$CURRENCY_CONFIG
				#mv offline_bench_result_gpu${DEVICE_ID}_${CURRENCY}.dat opt_bench_result_gpu${DEVICE_ID}_${CURRENCY}_${OPT_ALGO}_min_offline.dat
				save_call_command ./freq_optimization -d $DEVICE_ID -c $CURRENCY -a $OPT_ALGO --min_hashrate=0.8 \
					--max_iterations=6 --currency_config=$CURRENCY_CONFIG
				mv offline_bench_result_gpu${DEVICE_ID}_${CURRENCY}.dat opt_bench_result_gpu${DEVICE_ID}_${CURRENCY}_${OPT_ALGO}_mh_offline.dat
			else
				save_call_command ./freq_optimization -d $DEVICE_ID -c $CURRENCY -a $OPT_ALGO --use_online_bench=${WALLET_MAP[$CURRENCY]} \
					--max_mem_oc=$max_mem_oc --max_graph_oc=$max_graph_oc --currency_config=$CURRENCY_CONFIG
				mv online_bench_result_gpu${DEVICE_ID}_${CURRENCY}.dat opt_bench_result_gpu${DEVICE_ID}_${CURRENCY}_${OPT_ALGO}_online.dat
			fi
            sleep 60
		done
	done
}

function optimization_min_hashrate_test {
	local DEVICE_ID=$1
	local min_hashrate=$2
	local online=$3
	for CURRENCY in VTC ZEC;
	do
		for OPT_ALGO in HC SA NM;
		do
			if [[ online -eq 0 ]];
			then
				save_call_command ./freq_optimization -d $DEVICE_ID -c $CURRENCY -a $OPT_ALGO --min_hashrate=$min_hashrate \
					--max_iterations=20 --currency_config=$CURRENCY_CONFIG
				mv offline_bench_result_gpu${DEVICE_ID}_${CURRENCY}.dat opt_bench_result_gpu${DEVICE_ID}_${CURRENCY}_${OPT_ALGO}_mh_offline.dat
			else
				save_call_command ./freq_optimization -d $DEVICE_ID -c $CURRENCY -a $OPT_ALGO --min_hashrate=$min_hashrate --use_online_bench=${WALLET_MAP[$CURRENCY]} \
					--max_mem_oc=$max_mem_oc --max_graph_oc=$max_graph_oc --currency_config=$CURRENCY_CONFIG
				mv online_bench_result_gpu${DEVICE_ID}_${CURRENCY}.dat opt_bench_result_gpu${DEVICE_ID}_${CURRENCY}_${OPT_ALGO}_mh_online.dat
			fi
            sleep 60
		done
	done
}

function exhaustive_test {
	local DEVICE_ID=$1
	local online=$2
	for CURRENCY in VTC RVN BTX XMR ETH ZEC LUX;
	do
		if [[ online -eq 0 ]];
		then
			save_call_command ./freq_exhaustive -d $DEVICE_ID -c $CURRENCY \
				--mem_step=$mem_step --graph_idx_step=$graph_step --max_mem_oc=$max_mem_oc --max_graph_oc=$max_graph_oc --currency_config=$CURRENCY_CONFIG
			mv offline_bench_result_gpu${DEVICE_ID}_${CURRENCY}.dat exhaustive_bench_result_gpu${DEVICE_ID}_${CURRENCY}_offline.dat
		else
			save_call_command ./freq_exhaustive -d $DEVICE_ID -c $CURRENCY --use_online_bench=${WALLET_MAP[$CURRENCY]} \
				--mem_step=$mem_step --graph_idx_step=$graph_step --max_mem_oc=$max_mem_oc --max_graph_oc=$max_graph_oc --currency_config=$CURRENCY_CONFIG
			mv online_bench_result_gpu${DEVICE_ID}_${CURRENCY}.dat exhaustive_bench_result_gpu${DEVICE_ID}_${CURRENCY}_online.dat
		fi
		sleep 60
	done
}


pushd $BIN_DIR
for id in 1;
do
	optimization_test $id 0
done

popd

