#!/bin/bash

mkdir -p ../../benchmark_results/binarysearch/results
mkdir -p ../../benchmark_results/binarysearch/samples

OUTPUT_FILE_NAME=fssl_`date +%s`_$$.txt

SAMPLE_FILE=../../benchmark_results/binarysearch/samples/$OUTPUT_FILE_NAME
RESULT_FILE=../../benchmark_results/binarysearch/results/$OUTPUT_FILE_NAME

touch $SAMPLE_FILE
touch $RESULT_FILE

set -e

CLIENT=false
BENCH_PROG="../../build/tests/bench_bs"
BENCH_PROG_ARGS="-e 33554432 -o fssl "
SEARCHES=(1)
ITERS=(3 3 3)

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

for ((II=0; II<${#SEARCHES[*]}; II++));
do
	COMMAND="$BENCH_PROG $BENCH_PROG_ARGS -s ${SEARCHES[II]} -i ${ITERS[II]}"
	echo "COMMAND: $COMMAND"
	echo "COMMAND: $COMMAND" >> $RESULT_FILE
	eval "$COMMAND" >> $SAMPLE_FILE 2>> $RESULT_FILE
	if [ "$CLIENT" = true ] ; then
		sleep 5
	fi
done