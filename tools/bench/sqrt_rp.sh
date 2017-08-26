#!/bin/bash

mkdir -p ../../benchmark_results/rp/results
mkdir -p ../../benchmark_results/rp/samples

OUTPUT_FILE_NAME=sqrt_`date +%s`_$$.txt

SAMPLE_FILE=../../benchmark_results/rp/samples/$OUTPUT_FILE_NAME
RESULT_FILE=../../benchmark_results/rp/results/$OUTPUT_FILE_NAME

touch $SAMPLE_FILE
touch $RESULT_FILE

set -e

CLIENT=false
BENCH_PROG="../../build/tests/bench_rp"
BENCH_PROG_ARGS=" -m 4836 -q 15 -r 120 -s 12 -n 35476 -o sqrt "

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

COMMAND="OMP_THREAD_LIMIT=2 $BENCH_PROG $BENCH_PROG_ARGS"
echo "COMMAND: $COMMAND" >> $RESULT_FILE
echo "COMMAND: $COMMAND"
eval "$COMMAND" >> $SAMPLE_FILE 2>> $RESULT_FILE