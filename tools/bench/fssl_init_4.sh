#!/bin/bash

mkdir -p ../../benchmark_results/init/results
mkdir -p ../../benchmark_results/init/samples

OUTPUT_FILE_NAME=fssl_`date +%s`_$$.txt

SAMPLE_FILE=../../benchmark_results/init/samples/$OUTPUT_FILE_NAME
RESULT_FILE=../../benchmark_results/init/results/$OUTPUT_FILE_NAME

touch $SAMPLE_FILE
touch $RESULT_FILE

set -e

CLIENT=false
BENCH_PROG="../../build/tests/bench_floram_init"
BENCH_PROG_ARGS="-s 1 -o fssl -i 30 "
ELCTS=(2097152 4194304 8388608 16777216 33554432 67108864 134217728 268435456 536870912 1073741824 2147483648 4294967296 8589934592 17179869184)

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
	COMMAND="$BENCH_PROG $BENCH_PROG_ARGS -e ${ELCTS[II]}"
	echo "COMMAND: $COMMAND"
	echo "COMMAND: $COMMAND" >> $RESULT_FILE
	eval "$COMMAND" >> $SAMPLE_FILE 2>> $RESULT_FILE
	if [ "$CLIENT" = true ] ; then
		sleep 5
	fi
done