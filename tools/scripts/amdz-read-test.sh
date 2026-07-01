#!/bin/bash
if [ $# -ne 2 ]; then
    echo "2 arguments required!"
    echo "usage: amdz-read-test-gx.sh <blocksize> <dirname>"
    exit -1
fi
blocksize=$1
dirnam=$2
mkdir -p $dirnam
iternum=0
curblocknum=0
#let maxsizegx5=128*256*1024
let maxsizegx=33554432
let maxnumofblks=$maxsizegx/$blocksize-1
while true; do
    if [ $curblocknum -ne $maxnumofblks ]; then
        sudo dd if="/dev/accord-le" of="${dirnam}/${iternum}data${curblocknum}.bin" bs=$blocksize skip=$curblocknum count=1
        let curblocknum=$curblocknum+1
    else
        sudo dd if="/dev/accord-le" of="${dirnam}/${iternum}data${curblocknum}.bin" bs=$blocksize skip=$curblocknum 
        let iternum=$iternum+1
        curblocknum=0
    fi
done
