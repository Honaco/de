#!/bin/bash 
. ./acle-functions.sh

# amdz-gx-dump.sh [flag] <altera|bios|log|user|kc|os|db|inaf_compt> <file.name> <gx|gxm2|auto> <input_db_file_path>


amdz-gx-dump() {

source_file="/dev/accord-le"

#Если ноль, то работает стационарно. Активируется при наличии '+' в конце первого аргумента. Если активирован (значение 1), копирование файла происходит от указанного места до конца АМДЗ.
to_end_flag=0

basic_check_return="$( basic_check)"
if ! [ -z "$basic_check_return" ]; then
	echo "$basic_check_return"
	exit 1
fi

arg_err_message() {
    echo "Usage: amdz-gx-dump.sh [flag] <altera|bios|log|user|kc|os|db|inaf_compt> <file.name> <gx|gxm2|auto> <input_db_file_path>"
    echo "--First argument - the type of database that you want to read"
    echo "--Second argument - file name"
    echo "--Third argument[optional] - controller type"
    echo "--Fourth argument[optional] - if gx and gxm2 are specified, then you can pass the path to the file as the fourth argument"
    echo "[flag] is -f if you use several accords and auto mode"
}

if [ $# -eq 0 ]; then
    arg_err_message
    exit 1
fi

force=0
if [ $1 == "-f" ]; then
    echo "-f parameter enable" 
    force=1
    shift
fi

if [ $# -gt 4 ]; then
    arg_err_message
    exit 1
fi

userfilename=$2

acle_type=0
manually_chosen_type=0
case "$3" in
    gx)
        acle_type=1
        manually_chosen_type=1
        ;;
    gxm2)
        acle_type=2
        manually_chosen_type=1
        ;;
    *)
    	if [ "$3" == "auto" ] || [ $# -eq 2 ]; then
            acle_type_def $force
            acle_type=$?
            if [ $acle_type -eq 0 ]; then
                exit 1
            fi
        else
            arg_err_message
            exit 1
        fi
        ;;
esac

if [ $# -eq 4 ]; then 
	if [ $manually_chosen_type -ne 1 ]; then
		echo "<input_db_file_path> can be used only when you choose sectoring (gx|gxm2) by your own"
		arg_err_message
		exit 1
	fi
	
	source_file="$4"
fi

if [ $acle_type -eq 1 ] || [ $acle_type -eq 2 ]; then
    :;
else
    echo "Accord type defining error"
    exit 1
fi

bs="256k"
count=0
skip=0

# Функция выбора одного из двух вариантов в зависимости от переданного флага
#
# Использование: 
# choice_of_two flag first second
#
# Описание:
# Если flag имеет значение 1, то функция пишет first
# Если flag имеет значение 2, то функция пишет second
# При ином значении пишет ноль
# 
choice_of_two() {
    if [ $# -ne 3 ]; then
        echo "InvalidArgNum"
        return 1
    fi
    
    if [ $1 -eq 1 ]; then
        echo $2
    elif [ $1 -eq 2 ]; then
        echo $3
    else
        echo 0
    fi
    
    return 0
}

first_arg=$1
to_end_flag=`echo $1 | grep -c "+"`
first_arg=${first_arg%'+'*}

case "$first_arg" in 
    altera)
        count=$( choice_of_two $acle_type 4 16)
        skip=0
       	;;
    bios)
        count=1
        skip=$( choice_of_two $acle_type 4 16)
        ;;
    user)
        count=1
        skip=$( choice_of_two $acle_type 5 17)
        ;;
    kc)
        count=2
        skip=$( choice_of_two $acle_type 6 18)
        ;;
    log)
        count=1
        skip=$( choice_of_two $acle_type 8 20)
        ;;
    os)
        count=$( choice_of_two $acle_type 55 107)
        skip=$( choice_of_two $acle_type 9 21)
        ;;
    db)
    	count=4
    	skip=$( choice_of_two $acle_type 5 17)
        ;;
    inaf_compt)
    # В инафе clean.amdz структурно представляет собой как будто дамп gx до операционной системы (альтера, биос, БДшки и журнал), поэтому можно сделать дамп gx до операционной системы, пихнуть его в инаф и тестировать на инафе, без железа.
        count=9
        case $acle_type in
            1)
                skip=0
                ;;
            2)
                skip=12
                ;;
            *)
            	arg_err_message
            	exit 1
            	;;
        esac
    	;;
    *)
        arg_err_message
        exit 1
        ;;
esac

name="gx"
if [ $acle_type -eq 2 ]; then
    name="gxm2"
fi

#echo bs=$bs count=$count skip=$skip

if [ $to_end_flag -eq 0 ]; then
	sudo dd if=$source_file of="$userfilename" bs=$bs count=$count skip=$skip status=progress
elif [ $to_end_flag -eq 1 ]; then
	sudo dd if=$source_file of="$userfilename" bs=$bs skip=$skip status=progress
else
	echo "+ parameter fail: wrong value"
	arg_err_message
fi


echo "Finished"

}

amdz-gx-dump $*
