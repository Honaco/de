#!/bin/bash
. ./acle-functions.sh

if [ -z "$(command -v lspci)" ]; then
    echo "lspci not installed, please, install"
    exit 1
fi

if [ -z "$(command -v grep)" ]; then
    echo "grep not installed, please, install"
    exit 1
fi

pci_map=$(lspci -n | cut -f 3 -d ' ')
gx_vid="1795"
num_of_dev=0
proper_device_flag=0
echo "List of available devices:"
for var in $pci_map; do
	if [ "$(echo "$var" | cut -f 1 -d ':')" = $gx_vid ]; then
		amdz_name=$(my_find_amdz_name "$var")
		lspci_amdz=$(lspci -k | grep -A 1 "$var")
		amdz_device=$(echo "$lspci_amdz" | grep "$var")
		amdz_driver=$(echo "$lspci_amdz" | grep "driver")
		echo "$amdz_name"
		echo "$amdz_device"
		echo "$amdz_driver"
		num_of_dev=$(($num_of_dev + 1))
	fi
done

if [ $num_of_dev -eq 0 ]; then
	echo "Error! Accord devices not found!"
	exit 1
fi
