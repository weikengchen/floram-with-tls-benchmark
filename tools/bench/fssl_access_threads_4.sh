#!/bin/bash

mkdir -p ../../benchmark_results/access_threads/results
mkdir -p ../../benchmark_results/access_threads/samples

OUTPUT_FILE_NAME=fssl_`date +%s`_$$.txt

SAMPLE_FILE=../../benchmark_results/access_threads/samples/$OUTPUT_FILE_NAME
RESULT_FILE=../../benchmark_results/access_threads/results/$OUTPUT_FILE_NAME

touch $SAMPLE_FILE
touch $RESULT_FILE

set -e

CLIENT=false
BENCH_PROG="../../build/tests/bench_oram_write"
BENCH_PROG_ARGS=" -s 1 -o fssl "
ELS=(1024 1024 1024 1024 1024 32768 32768 32768 32768 32768 1048576 1048576 1048576 1048576 1048576 33554432 33554432 33554432 33554432 33554432 1073741824 1073741824 1073741824 1073741824 1073741824)
ITERS=(32 32 32 32 32 182 182 182 182 182 256 256 256 256 256 726 726 726 726 726 4096 4096 4096 4096 4096)
TCTS=(1 2 4 8 16 1 2 4 8 16 1 2 4 8 16 1 2 4 8 16 1 2 4 8 16)

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

for ((II=0; II<${#TCTS[*]}; II++));
do
	COMMAND="OMP_THREAD_LIMIT=${TCTS[II]} $BENCH_PROG $BENCH_PROG_ARGS -e ${ELS[II]} -i ${ITERS[II]}"
	echo "COMMAND: $COMMAND"
	echo "COMMAND: $COMMAND" >> $RESULT_FILE
	eval "$COMMAND" >> $SAMPLE_FILE 2>> $RESULT_FILE
	if [ "$CLIENT" = true ] ; then
		sleep 5
	fi
done