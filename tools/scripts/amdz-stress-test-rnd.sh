#!/bin/bash

pathToRndUtil=$1
numberOfBitesPerReq=$2
pathToOutputDir=$3

if ! [ -d $pathToOutputDir ]; then
	mkdir -p $pathToOutputDir
fi

padd=''
i=1
while true; do
	$pathToRndUtil $numberOfBitesPerReq | dd oflag=direct > $pathToOutputDir/Test$padd$i
	let i=$i+1
	if [ i == 0 ]
	then
	    padd+='a'
	fi
done
