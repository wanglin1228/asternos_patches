#!/usr/bin/python
from __future__ import print_function
try:

    import sys
    import os
    import logging
    import json
    import subprocess
    from subprocess import call
    import click
    import math
    import commands

except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

MAX_FAN_NUM = 3
MAX_PSU_NUM = 2
MAX_TEMP_NUM = 4

PSU_LIST = {
    0: ['PSU1', '/sys/bus/i2c/devices/5-0059/', '/sys/bus/i2c/devices/6-0040/X20XP_PSU/psu1_present'],
    1: ['PSU2', '/sys/bus/i2c/devices/4-0058/', '/sys/bus/i2c/devices/6-0040/X20XP_PSU/psu2_present']
}

FAN_LIST  = ['fan1_outlet_rpm', 'fan1_inlet_rpm', 'fan2_outlet_rpm', 'fan2_inlet_rpm', 'fan3_outlet_rpm', 'fan3_inlet_rpm']
TEMP_LIST = ['switch_lm75', 'cpu_lm75', 'fan_lm75_left', 'fan_lm75_right']

TEMP_PATH        = '/sys/bus/i2c/devices/6-0040/X20XP_Sensor/'
FAN_PATH         = '/sys/bus/i2c/devices/6-0040/X20XP_FAN/'



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

def sensors_temp():
    print('TEMPERATURE SENSORS:')
    for x in TEMP_LIST:
        result = get_attr_value(TEMP_PATH + x)
        temp = int(result) /2.0
        print("    {} is {} degrees(C)".format(x, temp))
    print('')
    return

def fan_speed():
    print('FAN SPEED:')
    for x in FAN_LIST:
        result = get_attr_value(FAN_PATH + x)
        speed = int(result) * 120
        print("    {} is {} RPM".format(x, speed))
    print('')
    return

def get_psu_power(index):
    PSU = PSU_LIST[index]
    if get_attr_value(PSU[2]) != '00':
        return 0
    else:
        result = get_attr_value(PSU[1]+'psu_pin')
        if result != 'ERR':
            return (int(result)/1000000.0)
        else:
            return 0

def psu_status():
    print("PSU_STATUS:")
    for index in range(0, MAX_PSU_NUM):
        PSU = PSU_LIST[index]
        print("    {}:".format(PSU[0]))
        if get_attr_value(PSU[2]) != '0':
            print('    not present')
        else:
            show_psu_status(PSU[1])
    return

def show_psu_status(path):
    # [model, vin, vout, fan_speed, temperature, pin, pout, iin, iout, max_iout]
    result_list = [0]*10

    result_list[0] = get_attr_value(path+"psu_mfr_model")
    result_list[1] = get_attr_value(path+"psu_vin")
    result_list[2] = get_attr_value(path+"psu_vout")
    result_list[3] = get_attr_value(path+"psu_fan_speed_1")
    result_list[4] = get_attr_value(path+"psu_temp_1")
    result_list[5] = get_attr_value(path+"psu_pin")
    result_list[6] = get_attr_value(path+"psu_pout")
    result_list[7] = get_attr_value(path+"psu_iin")
    result_list[8] = get_attr_value(path+"psu_iout")
    result_list[9] = get_attr_value(path+"psu_iout_max")
    if result_list[0] != 'ERR':
        print('    model: {}'.format(result_list[0]))
    
    if result_list[1] != 'ERR':
        vin = int(result_list[1])/1000.0
        print('    Input Voltage:  {:+3.2f} V'.format(vin))
        
    if result_list[2] != 'ERR':
        vout = int(result_list[2])/1000.0
        print('    Output Voltage:  {:+3.2f} V'.format(vout))
    
    if result_list[3] != 'ERR':
        fan_speed = int(result_list[3])
        print('    Fan Speed:      {:3d} RPM'.format(fan_speed))
    
    if result_list[4] != 'ERR':
        temperature = int(result_list[4])/1000.0
        print('    Temperature:    {:+3.1f} C'.format(temperature))
    
    if result_list[5] != 'ERR':
        pin = int(result_list[5])/1000000.0
        print('    Input Power:    {:3.2f} W'.format(pin))
    
    if result_list[6] != 'ERR':
        pout = int(result_list[6])/1000000.0
        print('    Output Power:   {:3.2f} W'.format(pout))
    
    if result_list[7] != 'ERR':
        iin = int(result_list[7])/1000.0
        print('    Input Current:  {:+3.2f} A'.format(iin))
    
    if result_list[8] != 'ERR':
        iout = int(result_list[8])/1000.0
        print('    Output Current: {:+3.2f} A'.format(iout),end='')
    
    if result_list[9] != 'ERR':
        max_iout = int(result_list[9])/1000.0
        print('  (max = {:+3.2f} A)'.format(max_iout))
        
    print('')
    return

# ==================== CLI commands and groups ====================

# This is our main entrypoint - the main 'environment' command
@click.group()
def cli():
    """environment - Command line utility for power voltage fans temps read set"""
    pass

# 'show' subgroup
@cli.group()
def show():
    """Display status of platform environment"""
    pass

# 'environment' subcommand

@show.command()
def fans():
    """Display Platform environment fans"""
    fan_speed()

@show.command()
def temps():
    """Display Platform environment temps"""
    sensors_temp()

# 'environment' subcommand
@show.command()
def powers():
    """Display Platform environment powers"""
    psu_status()

if __name__ == "__main__":
    cli()
