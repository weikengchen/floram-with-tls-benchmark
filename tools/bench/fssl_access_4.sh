#!/bin/bash

mkdir -p ../../benchmark_results/access/results
mkdir -p ../../benchmark_results/access/samples

OUTPUT_FILE_NAME=fssl_`date +%s`_$$.txt

SAMPLE_FILE=../../benchmark_results/access/samples/$OUTPUT_FILE_NAME
RESULT_FILE=../../benchmark_results/access/results/$OUTPUT_FILE_NAME

touch $SAMPLE_FILE
touch $RESULT_FILE

set -e

CLIENT=false
BENCH_PROG="../../build/tests/bench_oram_write"
BENCH_PROG_ARGS="-s 1024 -o fssl "
#ELCTS=(67108864 134217728 268435456 536870912 1073741824 2147483648 4294967296)
#ITERS=(1024 1450 2048 2898 4096 5794 8192)
ELCTS=(1048576)
ITERS=(128)


# The period is ceil(sqrt(ram->blockcount)/8)
# for 4096 -> 8
# for 65536 -> 32
# for 1048576 -> 128


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
