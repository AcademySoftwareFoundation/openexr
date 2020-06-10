#!/usr/bin/env bash

set -ex

LOGDIR="$1"

if [[ -d ${LOGDIR}/Testing/Temporary ]]; then 
	for log in ${LOGDIR}/Testing/Temporary/MemoryChecker.*.log; do 
		echo "========================== " ${log##*/} " ==========================" 
		cat $log 
		echo 
	done 
else 
	echo "Memcheck log files not found." 
fi
