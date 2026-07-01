#!/bin/bash
. ./acle-functions.sh

basic_check_return="$( basic_check)"
if ! [ -z "$basic_check_return" ]; then
	echo "$basic_check_return"
	exit 1
fi


# Flags
Accord_name="Accord GX"
force_local_utils=0
force=0
gx_pid_arr=(0700 0710 0715 0720 0725 0730)
current_dir=$(dirname "$(readlink -f "$0")")
offset=9
while [ -n "$1" ]
do
	case "$1" in
	--force-local-utils) 
		force_local_utils=1
		;;
	-f) 	
		force=1
		;;
	--help)
		echo "$0 - update OS image in AccordGX memory chip"
		echo "Usage: $0 [options] image_filename"
		echo "-f - forcibly clear accord even if several are found"
		echo "--force-local-utils - forcibly use utils from accord-le project"
		exit
		;;
	*) 
		echo "image file='$1'"
		image="$1"
		;;
	esac
	
	shift
done

# Check accord uniqueness 

accord_uniq_check "$Accord_name" "$force" "${gx_pid_arr[@]}"
if [ $? -eq 1 ]; then
	exit 1
fi

# Check if the system has necessary utility else run util from accord-le proj.

update_accord_firmware "$current_dir" $offset "$image"
