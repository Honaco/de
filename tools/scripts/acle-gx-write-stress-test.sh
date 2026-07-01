#!/bin/bash

if [ $# -ne 2 ]; then
    echo "Must be 2 parameters!"
    echo "usage: acle-gx-write-stress-test.sh <fileToWrite> <PathToExecWriteProgram>"
    exit -1
fi
# Flags
current_dir=$(dirname "$(readlink -f "$0")")
offset=4
FileToWrite="$1"
PathToExecWriteProgram="$2"
SizeOfFile=$( du -b Makefile | awk '{print $1}' )

if [ $SizeOfFile -gt 262144 ]; then
    echo "ERROR! File must be less than 256Kb"
fi

while true; do
    $PathToExecWriteProgram -o $offset "$FileToWrite"
    let offset=$offset+1
    if [ $offset -eq 128 ]; then
        offset=4
    fi
done
