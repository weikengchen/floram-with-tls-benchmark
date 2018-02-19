#!/usr/bin/env bash

function help {
 echo "$(basename $0) shows the maximum memory used by any running process, including daemons. 
Let it run to completion or Ctrl-C to terminate.
The process can be specified by name (case insensitive) or by PID. 
Non-java processes are matched against their full name, not including the path.

$(basename $0) has special matching for Java processes:
 a) It can match the command line for Java processes, however it may not select the proper 
    process if you just ask for 'java'; instead, provide a more unique part of the command to search for.
 b) A substring can be provided.

Examples:
  1) $(basename $0) firefox
  2) Play processes run as NettyServer, so to find out how much memory a Play webapp uses, type: 
     $(basename $0) netty"
  if [ "$1" ]; then echo -e "\n$1"; fi
  exit -1
}

if [ -z "$1" ]; then
  help "No PID or command name provided."
elif [[ "$1" =~ ^-?[0-9]+$ ]]; then
  pid="$1"
else # check for Java process
  PROC="$(jps -v | grep -i "$1" | head -n1)"
  pid="$(echo "$PROC" | cut -d' ' -f1)"
  if [ -z "$pid" ]; then # check for any process with the given string
    pid="$(pidof -s "$1")"
    if [ -z "$pid" ]; then 
      echo "No such process found"
      exit -2
    fi
    PROC="$(ps -h $pid | tr -s [:space:] ' ' | cut -d' ' -f5)"
    echo "Found non-Java process: $(ps -h $pid)"
  else
    echo "Found Java process: $(echo $PROC | cut -d' ' -f2)"
  fi
fi

MORE=true
CLR="\033[K"

function output {
  >&2 echo -en "$CLR"
  >&2 echo "$(echo "$OUTPUT" | sort -n | tail -n1 | numfmt --to=si)"
}

trap CTRL_C INT

function CTRL_C {
  unset MORE
  echo -en "\r$CLR"
}

while [ "$MORE" ]; do
  MORE=`ps -o vsz= $pid`
  MAX=`echo ${MORE}000 | numfmt --to=si`
  >&2 echo -en "$CLR $MAX \r"
  OUTPUT="${OUTPUT}000
$MORE"
  sleep 1
done 
output

