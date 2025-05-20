#!/usr/bin/python

from __future__ import print_function
from tabulate import tabulate
import os
import sys
import logging
import json

MAX_FAN_NUM = 5
MAX_PSU_NUM = 2
PSU_LIST = ['PSU1','PSU2'] #0x58, 0x59

THERMAL_SENSOR_LIST = ['NCT7511Y(U73)', 'G781(U94)', 'G781(U34)', 'G781(U4)']

PLATFORM_INSTALL_INFO_FILE  = '/etc/sonic/platform_install.json'
BMC_SYSFILE_PATH            = '/sys/class/hwmon/hwmon3/device/CX532P_SYS/'
FAN_SYSFILE_PATH            = '/sys/class/hwmon/hwmon3/device/CX532P_FAN/'
POWER_SYSFILE_PATH          = '/sys/class/hwmon/hwmon3/device/CX532P_PSU/'
THERMAL_SYSFILE_PATH        = '/sys/class/hwmon/hwmon3/device/CX532P_THERMAL/'

def get_psu_path():
    """
    get psu path when without BMC control
    """
    psu_path = []
    try:
        with open(PLATFORM_INSTALL_INFO_FILE) as fd:
            install_info = json.load(fd)
            for psu_name in PSU_LIST:
                psu = install_info[1][psu_name]
                psu_path.append(psu['path']+'/')
            return psu_path
    except Exception:
        print("Fail to get psu sysfsfile path")
    
    return psu_path

def get_thermal_sensor_path():
    sensor_path = []
    try:
        with open(PLATFORM_INSTALL_INFO_FILE) as fd:
            install_info = json.load(fd)
            for sensor_name in THERMAL_SENSOR_LIST:
                sensor = install_info[1][sensor_name]
                sensor_path.append(sensor['hwmon_path']+'/')
            return sensor_path
    except Exception:
        print("Fail to get sensor sysfsfile path")
        
    return sensor_path

# Get sysfs attribute
def get_attr_value(attr_path):
    retval = 'ERR'
    if not os.path.isfile(attr_path):
        return retval

    try:
        with open(attr_path, 'r') as fd:
            retval = fd.read()
    except Exception as error:
        logging.error("Unable to open ", attr_path, " file !")

    retval = retval.rstrip('\r\n')
    fd.close()
    return retval

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

def show_sensor_table():

    headers = ['Sensor', 'Temperature', 'High', 'Low', 'Critical High', 'Critical Low']
    table = list()
    temp = list()
    sensor_table = list()
    
    sensor_table = [
        ['Left Main Board'   , 'cpu_l_temp'],
        ['Right Main Board'    , 'cpu_r_temp'],
        ['Left Fan Board'    , 'fan_1_temp'],
        ['Right Fan Board'     , 'fan_2_temp'],
        ['Temp'      , 'temp'],
    ]

    for index in range(len(sensor_table)):
        name = sensor_table[index][0]
        for x in range(0, 1):
            sys_path = THERMAL_SYSFILE_PATH + sensor_table[index][x+1]
            t = get_attr_value(sys_path)
            if t == 'ERR':
                temp.append('N/A')
            else:
                if t.isdigit():
                    t = int(t)
                temp.append('{} C'.format(t))

        table.append([name, temp[0]])
        del temp[:]
    
    print(tabulate(table, headers, tablefmt='simple', stralign='right'))
    print('')

def fan_status(index):
    sys_path = FAN_SYSFILE_PATH + 'fan{}_status'.format(index)
    ret = get_attr_value(sys_path)
    if 'is Good' in ret:
        return 'OK'
    elif 'is Fail' in ret:
        return 'Fail'
    else:
        return 'N/A'
        
def fan_present(index):
    sys_path = FAN_SYSFILE_PATH + 'fan{}_present'.format(index)
    ret = get_attr_value(sys_path)
    if 'is present' in ret:
        return 'Present'
    else:
        return 'Not Present'

def fan_speed_dual(index):
    sys_path = FAN_SYSFILE_PATH + 'fan{}_speed_rpm'.format(index)
    front_ret = 'N/A'
    rear_ret = 'N/A'
    reg_file = open(sys_path, 'r')
    text = reg_file.readlines()
    for line in text:
        if 'FanModule{0} Front'.format(index) in line:
            front_ret = line.split(':')[1].strip() + ' RPM'
        elif 'FanModule{0} Rear'.format(index) in line:
            rear_ret = line.split(':')[1].strip() + ' RPM'
    
    return (front_ret, rear_ret)

def show_fan_table():
    headers = ['Fan', 'Speed', 'Presence', 'Status']
    table = []
    for index in range(1, MAX_FAN_NUM+1):
        name_front = "FAN{}-Front".format(index)
        name_rear = "FAN{}-Rear".format(index)
        speed_front, speed_rear = fan_speed_dual(index)
        present = fan_present(index)
        status = fan_status(index)
        table.append( [name_front, speed_front, present, status] )
        table.append( [name_rear , speed_rear, present, status] )
    
    print(tabulate(table, headers, tablefmt='simple', stralign='right'))
    print('')

def is_psu_present(psu_number):
    sys_path = POWER_SYSFILE_PATH + 'psu_present'
    search_str = "PSU {} is present".format(psu_number)
    if os.path.exists(sys_path):
       value = get_attr_value(sys_path)
       if search_str in value:
            return True
       else:
            return False
    
    return False

def show_psu_status(path):
    # [model, vin, vout, fan_speed, temperature, pin, pout, iin, iout, max_iout]
    result_list = [0]*10
    try:
        reg_file = open(path, 'r')
    except IOError as e:
        print( "Error: unable to open file: %s" % str(e))
        return False 
    
    text_lines = reg_file.readlines()
    reg_file.close()
    
    for line in text_lines:
        spline = line.split(' ')
        if "MFR_MODEL" in spline:
            result_list[0] = spline[-1]
        if "VIN" in spline:
            result_list[1] = spline[-1]
        if "VOUT" in spline:
            result_list[2] = spline[-1]
        if "FAN_SPEED" in spline:
            result_list[3] = spline[-1]
        if "TEMP_1" in spline:
            result_list[4] = spline[-1]
        if "PIN" in spline:
            result_list[5] = spline[-1]
        if "POUT" in spline:
            result_list[6] = spline[-1]
        if "IIN" in spline:
            result_list[7] = spline[-1]
        if "IOUT" in spline:
            result_list[8] = spline[-1]
        if "MFR_IOUT_MAX" in spline:
            result_list[9] = spline[-1]                
                    
    if result_list[1] != 'ERR':
        vin = int(result_list[1])/1000.0
        print ('    Input Voltage:  {:+3.2f} V'.format(vin))
        
    if result_list[2] != 'ERR':
        vout = int(result_list[2])/1000.0
        print ('    Output Voltage:  {:+3.2f} V'.format(vout))
    
    if result_list[3] != 'ERR':
        fan_speed = int(result_list[3])
        print ('    Fan Speed:      {:3d} RPM'.format(fan_speed))   
    
    if result_list[4] != 'ERR':
        temperature = int(result_list[4])/1000.0
        print ('    Temperature:    {:+3.1f} C'.format(temperature))    
    
    if result_list[5] != 'ERR':
        pin = int(result_list[5])
        print ('    Input Power:    {:3.2f} W'.format(pin))    
    
    if result_list[6] != 'ERR':
        pout = int(result_list[6])
        print ('    Output Power:   {:3.2f} W'.format(pout))    
    
    if result_list[7] != 'ERR':
        iin = int(result_list[7])/1000.0
        print ('    Input Current:  {:+3.2f} A'.format(iin))    
    
    if result_list[8] != 'ERR':
        iout = int(result_list[8])/1000.0
        print ('    Output Current: {:+3.2f} A'.format(iout),end='')    
    
    if result_list[9] != 'ERR':
        max_iout = int(result_list[9])/1000.0
        print ('  (max = {:+3.2f} A)'.format(max_iout))
        
    print('')
    return

def psu_status():
    psu_path = []
    for x in range(0,MAX_PSU_NUM):
        if is_psu_present(x+1):
            print("PSU{} present".format(x+1))
            show_psu_status(POWER_SYSFILE_PATH + 'psu{}_power'.format(x+1))

    return

def main():
    """
    Usage: %(scriptName)s command object

    command:
        fan_status     : display fans status(present/power good)
    """

    if len(sys.argv)<2:
        print (main.__doc__)

    for arg in sys.argv[1:]:
        if arg == 'fan_status':
            show_fan_table()
        elif arg == 'sensor_status':
            show_sensor_table()
        elif arg == 'power_status':
            psu_status()
        else:
            print (main.__doc__)

if __name__ == "__main__":
    main()
