#!/bin/bash

mkdir -p ../../benchmark_results/access/samples
mkdir -p ../../benchmark_results/access/results

OUTPUT_FILE_NAME=circuit_`date +%s`_$$.txt

SAMPLE_FILE=../../benchmark_results/access/samples/$OUTPUT_FILE_NAME
RESULT_FILE=../../benchmark_results/access/results/$OUTPUT_FILE_NAME

touch $SAMPLE_FILE
touch $RESULT_FILE

set -e

CLIENT=false
BENCH_PROG="../../build/tests/bench_oram_write"
BENCH_PROG_ARGS="-s 4096 -o circuit "
ELCTS=(16384)
ITERS=(10)
#ELCTS=(65536 65536 65536)
#ITERS=(10 10 10)
#ELCTS=(1048576)
#ITERS=(100)

while getopts ":c:y:z:" opt; do
	case $opt in
		c)
			BENCH_PROG_ARGS+=" -c $OPTARG "
			CLIENT=true
			;;
		y)
			BENCH_PROG_ARGS+=" -y $OPTARG "
			;;
		z)
			BENCH_PROG_ARGS+=" -z $OPTARG "
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
