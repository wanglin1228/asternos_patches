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

MAX_FAN_NUM = 2
MAX_TEMP_NUM = 1

FAN_LIST = ['fan1_outlet_rpm', 'fan2_outlet_rpm']
TEMP_LIST = ['ac5y_lm75']

TEMP_PATH        = '/sys/bus/i2c/devices/6-0040/X102S_XGT_Sensor/'
FAN_PATH         = '/sys/bus/i2c/devices/6-0040/X102S_XGT_FAN/'
SYS_PATH         = '/sys/bus/i2c/devices/6-0040/X102S_XGT_SYS/'



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
    for index in range(0, MAX_TEMP_NUM):
        sensor = TEMP_LIST[index] 
        result = get_attr_value(TEMP_PATH + sensor)
        temp = int(result) /2.0
        print("    {} is {} degrees(C)".format(sensor, temp))
    print('')
    return


def fan_speed():
    print('FAN SPEED:')
    for x in FAN_LIST:
        result = get_attr_value(FAN_PATH + x)
        speed = int(result) * 60
        print("    {} is {} RPM".format(x, speed))
    print('')
    return

def system_info():
    print('SYSTEM:')
    result = get_attr_value(SYS_PATH + 'cpld_version')
    print("     cpld version 0x{}".format(result))
    result = get_attr_value(SYS_PATH + 'board_version')
    print("     board version 0x{}".format(result))
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
    fan_speed();

@show.command()
def temps():
    """Display Platform environment temps"""
    sensors_temp();

@show.command()
def system():
    """Display Platform environment system"""
    system_info();

if __name__ == "__main__":
    cli()
