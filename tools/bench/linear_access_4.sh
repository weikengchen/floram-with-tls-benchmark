#!/bin/bash

mkdir -p ../../benchmark_results/access/results
mkdir -p ../../benchmark_results/access/samples

OUTPUT_FILE_NAME=linear_`date +%s`_$$.txt

SAMPLE_FILE=../../benchmark_results/access/samples/$OUTPUT_FILE_NAME
RESULT_FILE=../../benchmark_results/access/results/$OUTPUT_FILE_NAME

touch $SAMPLE_FILE
touch $RESULT_FILE

set -e

CLIENT=false
BENCH_PROG="../../build/tests/bench_oram"
BENCH_PROG_ARGS="-s 1 -o linear "
ELCTS=(32 64 128 256 512 1024 2048 4096 8192 16384 32768 65536 131072 262144 524288 1048576)
ITERS=(30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30)

while getopts ":c:" opt; do
	case $opt in
		c)
			BENCH_PROG_ARGS+=" -c $OPTARG "
			CLIENT=true
			;;
		\?)
			echo "Invalid option: -$OPTARG" >&2
			exit 1
			;;
		:)
			echo "Option -$OPTARG requires an argument." >&2
			exit 1
			;;
	esac
done

for ((II=0; II<${#ELCTS[*]}; II++));
do
	COMMAND="$BENCH_PROG $BENCH_PROG_ARGS -e ${ELCTS[II]} -i ${ITERS[II]}"
	echo "COMMAND: $COMMAND"
	echo "COMMAND: $COMMAND" >> $RESULT_FILE
	eval "$COMMAND" >> $SAMPLE_FILE 2>> $RESULT_FILE
	if [ "$CLIENT" = true ] ; then
		sleep 5
	fi
done