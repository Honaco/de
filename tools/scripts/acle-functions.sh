#/bin/bash

amdz_map="\
ACCORD-LE=1795:0700 \
ACCORD-GX=1795:0710 \
ACCORD-GXM=1795:0715 \
ACCORD-GXMH=1795:0720 \
ACCORD-GX2AE=1795:0725 \
ACCORD-GX2MH=1795:0730 \
ACCORD-GX5AE=1795:0735 \
"

# Проверка наличия драйвера и необходимых команд bash

basic_check() {
	if ! [ -e "/dev/accord-le" ] ; then
	    echo "accord-le not found, make sure the kernel module was successfully loaded"
	fi

	if [ -z "$(command -v lspci)" ]; then
	    echo "lspci not installed, please, install"
	fi

	if [ -z "$(command -v grep)" ]; then
	    echo "grep not installed, please, install"
	fi

	if [ -z "$(command -v whereis)" ]; then
	    echo "whereis not installed, please, install"
	fi
}

# Первый аргумент - pid рассматриваемого аккорда. Второй - массив из pid аккордов, являющихся правильными. Возвращает pid найденного аккорда в случае успеха и 2 в случае неудачи

determ_pid () {
        if [ $# -lt 2 ]; then
                echo 1
		return 1
        fi
	#echo num of args in determ_pid is $#
	
	flag=0
	local searched_pid="$1"
	local arguments=("$@")
	local gx_pid_arr="${arguments[@]:1}"
	#echo gx_pid_arr in determ_pid func: ${gx_pid_arr[@]} .
	#echo arguments in determ_pid func: ${arguments[@]}
	for pid in ${gx_pid_arr[@]}
	do
		#echo searched_pid: $searched_pid , pid: $pid
		if [ "$searched_pid" = "$pid" ]; then
			echo "$pid"
			flag=1
		fi
	done
	
	if [ $flag -eq 0 ]; then
		echo 2
	fi
}

# Проверка, с правильной ли серией аккорда работаем (GX или GXM2). Также проверка на наличие более 1 аккорда.
# Первый аргумент - название серии аккорда, с которой работаем.
# Второй аргумент - флаг force
# Третий аргумент - массив из pid аккордов данной серии вида (0710 0715 0720).

accord_uniq_check() {
	local Accord_name="$1"
	local force="$2"
	local arguments=("$@")
	local gx_pid_arr="${arguments[@]:2}"
	#echo arguments in uniq check: ${arguments[@]}
	#echo gx pid ar in accord uniq check: ${gx_pid_arr[@]}
	#echo "force = " $force
	pci_map=$(lspci -n | cut -f 3 -d ' ')
	gx_vid="1795"
	num_of_dev=0
	proper_device_flag=0
	for var in $pci_map; do
		if [ "$(echo "$var" | cut -f 1 -d ':')" = $gx_vid ]; then
		#echo "meow"
			num_of_dev=$(($num_of_dev + 1))
			val=$(echo "$var" | cut -f 2 -d ':')
			gx_pid=$( determ_pid "$val" "${gx_pid_arr[@]}")
			#echo "$gx_pid"
                        if [ "$gx_pid" = "1" ]; then
                                echo "Error: determ_pid function must get min 2 args"
                                return 1
			fi

			if [ "$gx_pid" = "2" ]; then
				if [ $force -eq 0 ]; then
					echo "Error: look like you using not $Accord_name version"
					return 1
				else
					echo "Not $Accord_name device found"
				fi
			else
				#echo "meow"
				echo "Your $Accord_name vid:pid - $gx_vid:$gx_pid"
				proper_device_flag=1
			fi
		fi
	done

	if [ $num_of_dev -eq 0 ] || [ $proper_device_flag -eq 0 ]; then
		echo "Error: $Accord_name not found!"
		return 1
	elif [ $num_of_dev -gt 1 ] && [ $force -ne 0 ]; then
		lspci_accord_driver=$(lspci -k | grep -B 1 "accord-le")
		echo "Warning! You have more than 1 $Accord_name in your PC and you are using -f flag"
		echo "Operations will be carried out with"
		echo "$lspci_accord_driver"
		echo "Are you sure?(y/n)"
		read answer
		case "$answer" in
	    		y|Y) echo "Continue..."
				;;
		    	n|N) echo "Terminated."
				return 1
				;;
		    	*) echo "Terminated."
		    		return 1
				;;
		esac
	elif [ $num_of_dev -gt 1 ]; then
		echo "Error: more than 1 $Accord_name found! Leave only one Accord or use -f param"
		return 1
	fi
}

# Чистит базу данных Аккорда
# Первый аргумент - директория, в которой лежит файл FF для затирания баз данных (с данной директорией должна соседствовать директория acle-write-sectors)
# Второй аргумент - массив из четырех адресов, на которых в данной серии аккорда лежит БД. Напр: (5 6 7 8) для GX и (17 18 19 20) для GXM2

clean_accord_db () {
	if [ $# -ne 5 ]; then
		echo "FAIL! clean_accord_db function require 5 args"
		return 1
	fi
	
	local current_dir="$1"
	local arguments=("$@")
	local db_addresses=("${arguments[@]:1}")
	#echo "$current_dir"
	echo ${db_addresses[0]}
	search_acle_write=$(whereis acle-write-sectors)
	if [ "$search_acle_write" = "acle-write-sectors:" ] || [ $force_local_utils -ne 0 ]; then
		if ! [ -x "$current_dir"/../acle-write-sectors/acle-write-sectors ]; then
			cd "$current_dir"/../acle-write-sectors || exit
			make
			cd "$current_dir" || exit
		fi
		"$current_dir"/../acle-write-sectors/acle-write-sectors -o ${db_addresses[0]} "$current_dir"/FF
		"$current_dir"/../acle-write-sectors/acle-write-sectors -o ${db_addresses[1]} "$current_dir"/FF
		"$current_dir"/../acle-write-sectors/acle-write-sectors -o ${db_addresses[2]} "$current_dir"/FF
		"$current_dir"/../acle-write-sectors/acle-write-sectors -o ${db_addresses[3]} "$current_dir"/FF
	else
		acle-write-sectors -o "${db_addresses[0]}" "$current_dir"/FF
		acle-write-sectors -o "${db_addresses[1]}" "$current_dir"/FF
		acle-write-sectors -o "${db_addresses[2]}" "$current_dir"/FF
		acle-write-sectors -o "${db_addresses[3]}" "$current_dir"/FF
	fi
}

# Аргумент - vid:pid рассматриваемого аккорда. Возвращает его название, если такой аккорд занесен в amdz_map

my_find_amdz_name() {
    local vid_pid="$1"
    for s in $amdz_map; do
        local map_vid_pid=$(echo "$s" | cut -d '=' -f 2)
        local amdz_name=$(echo "$s" | cut -d '=' -f 1)
        if [ $map_vid_pid == $vid_pid ]; then
            echo "$amdz_name"
        fi
    done
}

# Updates accord firmware
# 1 arg - directory accord-le/tools/scripts
# 2 arg - offset to write new firmware (9 for GX and 21 for GXM2)
# 3 arg - new firmware bin file

update_accord_firmware() {
	if [ $# -ne 3 ]; then
		echo "FAIL! update_accord_firmware function requires 3 args"
		return 1
	fi
	
	local current_dir="$1"
	local offset="$2"
	local image="$3"
	#echo "$current_dir"
	search_acle_write=$(whereis acle-write-sectors)
	if [ "$search_acle_write" = "acle-write-sectors:" ] || [ $force_local_utils -ne 0 ]; then
		if ! [ -x "$current_dir"/../acle-write-sectors/acle-write-sectors ]; then
			cd "$current_dir"/../acle-write-sectors || exit
			make
			cd "$current_dir" || exit
		fi
		
		"$current_dir"/../acle-write-sectors/acle-write-sectors --offset $offset "$image"
	else
		acle-write-sectors -o $offset "$image"
	fi
}

# Определяет, какой аккорд подключен к ЭВМ с драйвером accord-le
# Возвращает 
#           1, если gx
#           2, если gxm2
#           0, если нет аккордов или ни к одному не подключен драйвер

acle_type_def() {
	local force="$1"
	local gx_pid_arr=(0700 0710 0715 0720 0725 0730)
	local gxm2=(0735)
	pci_map=$(lspci -n | cut -f 3 -d ' ')
	gx_vid="1795"
	num_of_dev=0
	proper_device_flag=0
	lspci_accord_driver=$(lspci -k | grep -B 1 "accord-le")
	
	if [ -z "$lspci_accord_driver" ]; then
        echo "There is no device with accord-le driver!"
        return 0
    fi
    
	for var in $pci_map; do
		if [ "$(echo "$var" | cut -f 1 -d ':')" = $gx_vid ]; then
			num_of_dev=$(($num_of_dev + 1))
		fi
	done

	if [ $num_of_dev -gt 1 ] && [ $force -ne 0 ]; then
		echo "Warning! You have more than 1 Accord in your PC and you are using -f flag"
		echo "Operations will be carried out with"
		echo "$lspci_accord_driver"
		echo "Are you sure?(y/n)"
		read answer
		case "$answer" in
	    		y|Y) echo "Continue..."
				;;
		    	n|N) echo "Terminated."
                    return 0
				;;
		    	*) echo "Terminated."
		    		return 0
				;;
		esac
	elif [ $num_of_dev -gt 1 ]; then
		echo "Error: more than 1 Accord found! Leave only one Accord or use -f param"
		return 0
	fi
	
    if [ "$(echo "$lspci_accord_driver" | grep Driver | cut -f 5 -d ':')" = "0735" ]; then
        return 2
    else
        return 1
    fi
}

