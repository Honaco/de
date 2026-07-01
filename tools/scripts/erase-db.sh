#!/bin/bash

if ! [ -e "/dev/accord-le" ] ; then
    echo "accord-le not found, make sure the kernel module was successfully loaded"
    exit 1
fi

# https://redmine.okbsapr.ru/projects/accord-le/wiki/%D0%92%D0%B8%D0%B4%D1%8B_LE
# У gx и gx2 одна разметка:
# https://redmine.okbsapr.ru/projects/accord-le/wiki/DataFlashStructure
# У gx5 - другая (странно, но по ссылке пока что - типы, а не сами размеры):
# https://redmine.okbsapr.ru/projects/accord-le/wiki/DataFlashStructreGx5
if [ -z $(command -v lspci) ]; then
    echo "lspci not installed, please, install"
    exit 1
fi
pci_map=$(lspci -n)

# Фильтруем по vid/pid:
# https://redmine.okbsapr.ru/projects/accord-le/wiki/%D0%92%D0%B8%D0%B4%D1%8B_LE
my_find_amdz_name() {
    vid_pid="$1"
    map="$2"
    for s in $map; do
        map_vid_pid=$(echo "$s" | cut -d '=' -f 2)
        amdz_name=$(echo "$s" | cut -d '=' -f 1)
        #echo $map_vid_pid
        #echo $amdz_name
        if [ $map_vid_pid == $vid_pid ]; then
            echo "$amdz_name"
        fi
    done
}
# порядок подсматривать в driver/accord-le.c
amdz_map1="\
ACCORD-LE=1795:0700 \
ACCORD-GX=1795:0710 \
ACCORD-GXM=1795:0715 \
ACCORD-GXMH=1795:0720 \
ACCORD-GX2AE=1795:0725 \
ACCORD-GX2MH=1795:0730 \
"
amdz_map2="\
ACCORD-GX5AE=1795:0735 \
"

SUCCESS_MSG=$(
    echo "$pci_map" | while IFS='' read -r pci_entry || [ -n "$pci_entry" ] ; do
        pci_vid_pid=$(echo "$pci_entry" | cut -d ' ' -f 3)

        amdz_name_for_layout1=$(my_find_amdz_name "$pci_vid_pid" "$amdz_map1")
        if [ -n "$amdz_name_for_layout1" ] ; then
            echo "Found ${amdz_name_for_layout1}, layout 1"
            break
        fi

        amdz_name_for_layout2= $(my_find_amdz_name "$pci_vid_pid" "$amdz_map2")
        if [ -n "$amdz_name_for_layout2" ] ; then
            echo "Found ${amdz_name_for_layout2}, layout 2"
            break
        fi

    done
)

if [ -z "$SUCCESS_MSG" ] ; then
    echo "No acceptable devices were found!"
    exit -1
fi

LAYOUT=$(echo "$SUCCESS_MSG" | rev | cut -d ' ' -f 1)
echo "$SUCCESS_MSG"

# Сама работа

current_dir=$(dirname $(readlink -f "$0"))

# Более-менее свежие прошивки возвращают memtable, по которому можно посмотреть
# соответствие области памяти и её типа. Тогда это можно распечатать с помощью
# `tools/acle-get-state`
if [ $LAYOUT == "1" ] ; then
    $current_dir/../acle-write-sectors/acle-write-sectors -o 5 $current_dir/FF
    $current_dir/../acle-write-sectors/acle-write-sectors -o 6 $current_dir/FF
    $current_dir/../acle-write-sectors/acle-write-sectors -o 7 $current_dir/FF
    $current_dir/../acle-write-sectors/acle-write-sectors -o 8 $current_dir/FF
elif [ $LAYOUT == "2" ] ; then
    $current_dir/../acle-write-sectors/acle-write-sectors -o 17 $current_dir/FF
    $current_dir/../acle-write-sectors/acle-write-sectors -o 18 $current_dir/FF
    $current_dir/../acle-write-sectors/acle-write-sectors -o 19 $current_dir/FF
    $current_dir/../acle-write-sectors/acle-write-sectors -o 20 $current_dir/FF
fi

