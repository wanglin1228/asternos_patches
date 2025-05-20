#!/bin/bash

MAX_POWER_150W="0x00 0x96"
MAX_POWER_370W="0x01 0x72"
MAX_POWER_720W="0x02 0xd0"
MAX_POWER_740W="0x02 0xe4"
MAX_POWER_1440W="0x05 0xa0"
MAX_POWER_2880W="0x0b 0x40"

platform=$1
device_sku_basedir="/usr/share/sonic/device"
device_sku=$(cat ${device_sku_basedir}/${platform}/default_sku | awk '{print $1}')
#device_sku=$(sudo decode-syseeprom -p)
device_sku_prefix=${device_sku%%-*}
echo "Init poe for sku "$device_sku
log_file=/var/log/poe.log
now=$(date)
echo $now > $log_file
echo $device_sku >> $log_file

if [ $device_sku == "CX204Y-24GT-HPW1-M-AC" -o $device_sku == "CX204Y-24GT-M-SWP2" ]
then
    sudo API_BT_Share_workspace -i 24 >> $log_file 2>&1       #CX204Y-24GT-M-SWP2

elif [ $device_sku == "CX204Y-24GT-HPW2-M-AC" -o $device_sku == "CX204Y-24GT-M-SWP4" ]
then
    sudo API_BT_Share_workspace -i 24 >> $log_file 2>&1  #CX204Y-24GT-M-SWP2
    sleep 0.03
    sudo API_BT_Share_workspace 0x00 0x03 0x07 0x0b 0x57 0x0c $MAX_POWER_740W 0x02 0x49 0x01 0xe0 0x0a >> $log_file 2>&1

elif [ $device_sku == "CX204Y-48GT-HPW2-M-AC" -o $device_sku == "CX204Y-48GT-M-SWP4" ]
then
    sudo API_BT_Share_workspace -i 204-48 >> $log_file 2>&1  #CX204Y-48GT-M-SWP4

elif [ $device_sku == "CX206Y-48GT-HPW4-M" -o $device_sku == "CX206Y-48GT-M-HWP4" ]
then
    sudo API_BT_Share_workspace -i 206-48 >> $log_file 2>&1  #CX206Y-48GT-M-HWP4 & CX206Y-48GT-M-HWP8

elif [ $device_sku == "CX206Y-48GT-M-HWP8" ]
then
    sudo API_BT_Share_workspace -i 206-48 >> $log_file 2>&1  #CX206Y-48GT-M-HWP4 & CX206Y-48GT-M-HWP8
    sleep 0.03
    sudo API_BT_Share_workspace 0x00 0x03 0x07 0x0b 0x57 0x0c $MAX_POWER_1440W 0x02 0x49 0x01 0xe0 0x0a >> $log_file 2>&1
    sleep 0.03
    sudo API_BT_Share_workspace 0x00 0x03 0x07 0x0b 0x57 0x0d $MAX_POWER_2880W 0x02 0x49 0x01 0xe0 0x0a >> $log_file 2>&1
elif [ $device_sku_prefix == "CX102S" ] && [ -z "${device_sku##*"HPW"*}" -o -z "${device_sku##*"SWP"*}" ]
then
    echo "Init poe for 102s "$device_sku
    sudo API_BT_Share_workspace -i 102s >> $log_file 2>&1  #CX102S-*
    sleep 0.03
    sudo API_BT_Share_workspace 0x00 0x03 0x07 0x0b 0x57 0x0c $MAX_POWER_150W 0x02 0x49 0x01 0xe0 0x0a >> $log_file 2>&1
fi

sleep 0.03
sudo API_BT_Share_workspace 0x01 0x18 0x06 0x0f 0x4e 0x4e 0x4e 0x4e 0x4e 0x4e 0x4e 0x4e 0x4e >> $log_file 2>&1 #save poe system config

