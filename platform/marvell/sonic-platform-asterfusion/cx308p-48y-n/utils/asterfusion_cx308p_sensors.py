#!/usr/bin/python

from __future__ import print_function
import os
import sys
import logging
import subprocess

MAX_FAN_NUM = 4
MAX_PSU_NUM = 2
PSU_LIST = ['PSU1','PSU2'] 

FAN_SYSFILE_PATH = '/sys/class/hwmon/hwmon2/device/CX308P_FAN/'
THERMAL_SYSFILE_PATH = '/sys/class/hwmon/hwmon2/device/CX308P_THERMAL/'
POWER_SYSFILE_PATH = '/sys/class/hwmon/hwmon2/device/CX308P_PSU/'

def get_hwmon_path():
    '''Find hwmon path. Return empty string if not found. Return first path if found more than one path.'''
    # limit maxdepth is necessary, or we may get infinite results because of loop symbol link(s)
    # redirect stderr to blackhole is necessary, or we will get something like "File system loop detected"
    cmd = ["find -L /sys/class/hwmon/ -maxdepth 3 -type d -name CX308P_FAN 2>/dev/null"]
    find = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    output, err = find.communicate()
    if type(output) == bytes:
        output = output.decode()
    output = output.strip()
    if len(output) < 1:
        return ""
    # Here we return the upper path instead of detailed path. e.g. '/sys/class/hwmon/hwmon2/device/'
    return "/".join(output.split("\n")[0].split("/")[:-1]) + "/"

def print_attr_value_lines(sys_path):
    retval = 'ERR'
    if not os.path.isfile(sys_path):
        return retval
    try:
        fo = open(sys_path, "r")
    except Exception as error:
        logging.error("Unable to open ", sys_path, " file !")
    for line in fo.readlines():
        line = line.strip()
        print ("    %s" % line)
    fo.close()
    return retval

def fan_status():
    sys_path = FAN_SYSFILE_PATH + 'fan_status'
    print ('FAN STATUS:')
    print_attr_value_lines(sys_path)
    return

def fan_present():
    sys_path = FAN_SYSFILE_PATH + 'fan_present'
    print ('FAN PRESENT:')
    print_attr_value_lines(sys_path)
    return

def fan_airflow():
    sys_path = FAN_SYSFILE_PATH + 'fan_airflow'
    print ('FAN AIRFLOW:')
    print_attr_value_lines(sys_path)
    return

def fan_speed():
    sys_path = FAN_SYSFILE_PATH + 'fan_speed_rpm'
    print ('FAN SPEED:')
    print_attr_value_lines(sys_path)
    return

def sensors_temp():
    print ('SENSOR TEMPERATURE:')
    sys_path = THERMAL_SYSFILE_PATH + 'thermal_sersor_1'
    print_attr_value_lines(sys_path)
    sys_path = THERMAL_SYSFILE_PATH + 'thermal_sersor_2'
    print_attr_value_lines(sys_path)
    sys_path = THERMAL_SYSFILE_PATH + 'thermal_sersor_3'
    print_attr_value_lines(sys_path)
    sys_path = THERMAL_SYSFILE_PATH + 'thermal_sersor_4'
    print_attr_value_lines(sys_path)
    return

def psu_present():
    sys_path = POWER_SYSFILE_PATH + 'psu_present'
    print ('PSU PRESENT:')
    print_attr_value_lines(sys_path)
    return

def psu_power_good():
    sys_path = POWER_SYSFILE_PATH + 'psu_status'
    print ('PSU POWER GOOD:')
    print_attr_value_lines(sys_path)
    return

def psu_info():
    for x in range(0,MAX_PSU_NUM):
        sys_path = POWER_SYSFILE_PATH + 'psu_module_{}'.format(x+1)
        print("PSU{} INFORMATION:".format(x+1))
        print_attr_value_lines(sys_path)

    return

def main():
    """
    Usage: %(scriptName)s command object

    command:
        fan_status     : display fans status(present/airflow/speed)
        sensor_status  : display temprature sensors information
        power_status   : display PSU status(present/power good/physical info)
    """

    if len(sys.argv)<2:
        print(main.__doc__)
        return

    global FAN_SYSFILE_PATH
    global THERMAL_SYSFILE_PATH
    global POWER_SYSFILE_PATH
    # Check whether hwmon path exists. If not, try find and update.
    if not os.path.exists(FAN_SYSFILE_PATH) or not os.path.exists(THERMAL_SYSFILE_PATH) or not os.path.exists(POWER_SYSFILE_PATH):
        sysfile_path = get_hwmon_path()
        if len(sysfile_path) <= 0:
            logging.error("Can't find sysfs hwmon directory. Please check platform driver.")
            return
        FAN_SYSFILE_PATH     = sysfile_path + "CX308P_FAN/"
        THERMAL_SYSFILE_PATH = sysfile_path + "CX308P_THERMAL/"
        POWER_SYSFILE_PATH   = sysfile_path + "CX308P_PSU/"

    for arg in sys.argv[1:]:
        if arg == 'fan_status':
            fan_status()
            fan_present()
            fan_airflow()
            fan_speed()
        elif arg == 'sensor_status':
            sensors_temp()
        elif arg == 'power_status':
            psu_present()
            psu_power_good()
            psu_info( )
        else:
            print (main.__doc__)

if __name__ == "__main__":
    main()
