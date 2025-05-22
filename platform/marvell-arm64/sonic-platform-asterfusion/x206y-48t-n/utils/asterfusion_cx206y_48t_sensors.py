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

MAX_FAN_NUM = 4
MAX_PSU_NUM = 2
MAX_TEMP_NUM = 3
MAX_VOL_NUM = 9
MAX_PSE_NUM = 12
MAX_PSE_PORT_NUM = 4

global total_poe_power
total_poe_power = 0

PSU_LIST = {
    0: ['PSU1', '/sys/class/hwmon/hwmon1/device/', '/sys/bus/i2c/devices/3-0075/psu_1_present'],
    1: ['PSU2', '/sys/class/hwmon/hwmon2/device/', '/sys/bus/i2c/devices/3-0075/psu_2_present']
}
FAN_LIST = {
    0:['FAN1', 'fan2'],
    1:['FAN2', 'fan3'],
    2:['FAN3', 'fan4'],
    3:['FAN4', 'fan5']
}
TEMP_LIST = {
    0: ['LM75B / MAC side', 'lm75_48_temp', 'lm75_48_temp_alert'],
    1: ['LM75B / PCB side', 'lm75_49_temp', 'lm75_49_temp_alert'],
    2: ['LM75B / FAN side', 'lm75_4a_temp', 'lm75_4a_temp_alert']  
}
VOL_LIST = {
    0: ['Monitor P0.1 (12V) Voltage', 'ADC1_vol', 'ADC1_under_alert', 'ADC1_over_alert'],
    1: ['Monitor P0.2 (3V) Voltage', 'ADC2_vol', 'ADC2_under_alert', 'ADC2_over_alert'],
    2: ['Monitor P0.6 (1.5V) Voltage', 'ADC3_vol', 'ADC3_under_alert', 'ADC3_over_alert'],
    3: ['Monitor P0.7 (1.8V) Voltage', 'ADC4_vol', 'ADC4_under_alert', 'ADC4_over_alert'],
    4: ['Monitor P1.0 (1.2V) Voltage', 'ADC5_vol', 'ADC5_under_alert', 'ADC5_over_alert'],
    5: ['Monitor P1.1 (1.15V) Voltage', 'ADC6_vol', 'ADC6_under_alert', 'ADC6_over_alert'],
    6: ['Monitor P1.2 (3.3V_STB) Voltage', 'ADC7_vol', 'ADC7_under_alert', 'ADC7_over_alert'],
    7: ['Monitor P1.5 (0.8V) Voltage', 'ADC8_vol', 'ADC8_under_alert', 'ADC8_over_alert'],
    8: ['Monitor P1.6 (2.5V) Voltage', 'ADC9_vol', 'ADC9_under_alert', 'ADC9_over_alert']
}

PSE_LIST = {
    0: ['PSE1_0', '1-0022'],
    1: ['PSE1_1', '1-0023'],
    2: ['PSE2_0', '1-0024'],
    3: ['PSE2_1', '1-0025'],
    4: ['PSE3_0', '1-0026'],
    5: ['PSE3_1', '1-0027'],
    6: ['PSE4_0', '1-0028'],
    7: ['PSE4_1', '1-0029'],
    8: ['PSE5_0', '1-002c'],
    9: ['PSE5_1', '1-002d'],
    10: ['PSE6_0', '1-0030'],
    11: ['PSE6_1', '1-0031']
}

PSE_PORT_LIST = {
    0: [1],
    0: [2],
    0: [3],
    0: [4]
}

MCU_SYSFILE_PATH        = '/sys/bus/i2c/devices/2-0070/'
SYSFILE_PATH            = '/sys/bus/i2c/devices/'
SYS_PATH                = '/sys/bus/i2c/devices/3-0075/'



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
        sensor_temp = get_attr_value(MCU_SYSFILE_PATH + sensor[1])
        print('    ' + sensor[0] + ': temp = ' + sensor_temp + ' degrees(C)')
    print('')
    return

def sensors_temp_alert():
    print('TEMPSENSORS OVER ALERT:')
    for index in range(0, MAX_TEMP_NUM):
        sensor = TEMP_LIST[index]
        sensor_alert = 'normal' if(get_attr_value(MCU_SYSFILE_PATH + sensor[2]) == '0') else 'alert'
        print('    ' + sensor[0] + ': status is ' + sensor_alert)
    print('')
    return


def smartFan_status():
    print('SMARTFAN STATUS:')
    fan_pwm = get_attr_value(MCU_SYSFILE_PATH + 'fan_pwm')
    smart_fan_enable = 'enable' if(get_attr_value(MCU_SYSFILE_PATH + 'smartFan_enable') == '1') else 'disable'
    #smartFan_setting = get_attr_value(MCU_SYSFILE_PATH + 'smartFan_setting_enable')
    smartFan_device_index = get_attr_value(MCU_SYSFILE_PATH + 'smartFan_device')
    smartFan_device = 'LM75B/MAC_side' if(smartFan_device_index == '0') else \
                      'LM75B/PCB_side' if(smartFan_device_index == '1') else \
                      'LM75B/FAN_side' if(smartFan_device_index == '2') else \
                      'N/A'
    smartFan_update = get_attr_value(MCU_SYSFILE_PATH + 'smartFan_update')
    smartFan_temp_max = get_attr_value(MCU_SYSFILE_PATH + 'smartFan_max_temp')
    smartFan_temp_mid = get_attr_value(MCU_SYSFILE_PATH + 'smartFan_mid_temp')
    smartFan_temp_min = get_attr_value(MCU_SYSFILE_PATH + 'smartFan_min_temp')
    smartFan_pwm_max = get_attr_value(MCU_SYSFILE_PATH + 'smartFan_max_pwm')
    smartFan_pwm_mid = get_attr_value(MCU_SYSFILE_PATH + 'smartFan_mid_pwm')
    smartFan_pwm_min = get_attr_value(MCU_SYSFILE_PATH + 'smartFan_min_pwm')
    print('    smart_fan_enable is ' + smart_fan_enable)
    #print('smartFan_setting is ' + smartFan_setting)
    print('    smartFan_device is ' + smartFan_device)
    print('    smartFan_update_degree is ' + smartFan_update + ' degrees(C)')
    
    print('    fan_pwm is ' + fan_pwm + '%')
    print('    smartFan_pwm_max is ' + smartFan_pwm_max + '%')
    print('    smartFan_pwm_mid is ' + smartFan_pwm_mid + '%')
    print('    smartFan_pwm_min is ' + smartFan_pwm_min + '%')

    print('    smartFan_temp_max is ' + smartFan_temp_max + ' degrees(C)')
    print('    smartFan_temp_mid is ' + smartFan_temp_mid + ' degrees(C)')
    print('    smartFan_temp_min is ' + smartFan_temp_min + ' degrees(C)')

    print('')
    return


def fan_status():
    print('FAN PRENSENT STATUS:')
    for index in FAN_LIST:
        x = FAN_LIST[index]
        fan_status = 'present' if(get_attr_value(MCU_SYSFILE_PATH + x[1] + '_present') == '0') else 'absent'
        print('    ' + x[0] + ' is ' + fan_status)
    print('')
    return

def fan_airflow():
    print('FAN AIRFLOW STATUS:')
    for index in FAN_LIST:
        x = FAN_LIST[index]
        fan_airflow_status = 'back-to-front' if(get_attr_value(MCU_SYSFILE_PATH + x[1] + '_airflow_dir') == '0') else 'front-to-back'
        print('    ' + x[0] + ': ' + fan_airflow_status)
    print('')
    return


def fan_speed():
    print('FAN SPEED:')
    for index in FAN_LIST:
        x = FAN_LIST[index]
        fan_outer_speed = get_attr_value(MCU_SYSFILE_PATH + x[1] + '_outer_rpm')
        fan_inner_speed = get_attr_value(MCU_SYSFILE_PATH + x[1] + '_inner_rpm')
        print('    ' + x[0] + ': outer speed is ' + fan_outer_speed + 'rpm' + ', inner speed is ' + fan_inner_speed +'rpm')
    print('')
    return

def fan_alert():
    print('FAN ALERT STATUS:')
    for index in FAN_LIST:
        x = FAN_LIST[index]
        fan_alert_status = 'normal' if(get_attr_value(MCU_SYSFILE_PATH + x[1] + '_status_alert') == '0') else 'alert'
        Fan_Not_Connect = 'normal' if(get_attr_value(MCU_SYSFILE_PATH + x[1] + '_notconnect_alert') == '0') else 'alert'
        Fan_Inner_RPM_Zero = 'normal' if(get_attr_value(MCU_SYSFILE_PATH + x[1] + '_innerRPMZero_alert') == '0') else 'alert'
        Fan_Inner_RPM_Under = 'normal' if(get_attr_value(MCU_SYSFILE_PATH + x[1] + '_innerRPMUnder_alert') == '0') else 'alert'
        Fan_Inner_RPM_Over = 'normal' if(get_attr_value(MCU_SYSFILE_PATH + x[1] + '_innerRPMOver_alert') == '0') else 'alert'
        Fan_Outer_RPM_Zero = 'normal' if(get_attr_value(MCU_SYSFILE_PATH + x[1] + '_outerRPMZero_alert') == '0') else 'alert'
        Fan_Outer_RPM_Under = 'normal' if(get_attr_value(MCU_SYSFILE_PATH + x[1] + '_outerRPMUnder_alert') == '0') else 'alert'
        Fan_Outer_RPM_Over = 'normal' if(get_attr_value(MCU_SYSFILE_PATH + x[1] + '_outerRPMOver_alert') == '0') else 'alert'
        Fan_Wrong_Airflow = 'normal' if(get_attr_value(MCU_SYSFILE_PATH + x[1] + '_wrongAirflow_alert') == '0') else 'alert'
        print('    ' + x[0] + ': alert status is ' + fan_alert_status)
        print('        ' + 'Fan_Not_Connect is ' + Fan_Not_Connect)
        print('        ' + 'Fan_Inner_RPM_Zero is ' + Fan_Inner_RPM_Zero)
        print('        ' + 'Fan_Inner_RPM_Under is ' + Fan_Inner_RPM_Under)
        print('        ' + 'Fan_Inner_RPM_Over is ' + Fan_Inner_RPM_Over)
        print('        ' + 'Fan_Outer_RPM_Zero is ' + Fan_Outer_RPM_Zero)
        print('        ' + 'Fan_Outer_RPM_Under is ' + Fan_Outer_RPM_Under)
        print('        ' + 'Fan_Outer_RPM_Over is ' + Fan_Outer_RPM_Over)
        print('        ' + 'Fan_Wrong_Airflow is ' + Fan_Wrong_Airflow)
    print('')
    return

def adc_status():
    print("ADC_VOLTAGE:")
    for index in range(0, MAX_VOL_NUM):
        adc = VOL_LIST[index]
        print('    ' + adc[0] + ': ' + get_attr_value(MCU_SYSFILE_PATH + adc[1]) + 'V')
    print('')
    return

def adc_alert_status():
    print("ADC_VOLTAGE_ALERT:")
    for index in range(0, MAX_VOL_NUM):
        adc = VOL_LIST[index]
        print('    ' + adc[0] + ': under_alert=' + get_attr_value(MCU_SYSFILE_PATH + adc[2]) + ', over_alert=' + get_attr_value(MCU_SYSFILE_PATH + adc[3]))
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
        print('    ' + PSU[0] + ':')
        if get_attr_value(PSU[2]) != '00':
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

def show_pse_status(pse_index, path):
    global total_poe_power
    pse_input_voltage  = get_attr_value(path + 'pse_input_voltage')
    if pse_input_voltage != 'ERR':
        vin = int(pse_input_voltage, 16) / 1000.0 * 3.662           #VOL = N * 3.662(mV), N get from register
        print('    Input Voltage:  {:+3.2f} V'.format(vin))

    pse_temperature  = get_attr_value(path + 'pse_temperature')
    if pse_temperature != 'ERR':
        temperature = -20.0 + int(pse_temperature ,16) * 0.652      #TEMP = -20 + N * 0.652(mV), N get from register
        print('    Temperature:    {:+3.1f} C'.format(temperature))  

    for port_index in range (0, MAX_PSE_PORT_NUM):
        port_num = pse_index * 4 + port_index
        port_curr = get_attr_value(path + 'pse_port' + str(port_index + 1) + '_current')
        port_vol = get_attr_value(path + 'pse_port'+ str(port_index + 1) + '_voltage')

        if port_curr != 'ERR' and port_vol != 'ERR':
            curr = int(port_curr, 16) * 61.035 / 1000000.0          #CURR = N * 61.035(uA), N get from register
            vol  = int(port_vol, 16) / 1000.0 * 3.662
            power = curr * vol
            print('    Ethernet' + str(port_num+1) + ': current = {:+3.3f} A'.format(curr) + ', voltage = {:+3.3f} V'.format(vol) + ',  power = {:+3.2f} W'.format(power))
            total_poe_power += power
    print('')
    return

def pse_status():
    global total_poe_power
    total_poe_power = 0
    print("PSE STATUS:")
    for index in range (0, MAX_PSE_NUM):
        PSE = PSE_LIST[index]
        print('    ' + PSE[0] + ':')
        PATH = SYSFILE_PATH + PSE[1] + '/'
        show_pse_status(index, PATH)

    print('total_poe_power = {:3.2f} W'.format(total_poe_power))
    total_power = get_psu_power(0) + get_psu_power(1)
    if total_power > 0:
        poe_power_radio = total_poe_power * 100.0 / total_power
        print('the poe power radio is {:3.2f}'.format(total_poe_power) + ' / {:3.2f}'.format(total_power) +' = {:3.2f} %'.format(poe_power_radio))

    else:
        print('error getting psu power')
    print('')
    return

def system_info():
    print ('SYSTEM:')
    result = get_attr_value(SYS_PATH + 'cpld_sw_version')
    print ("     cpld version 0x{}".format(result))
    print ('')
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
def powers():
    """Display Platform environment powers"""
    psu_status();

@show.command()
def voltages():
    """Display Platform environment voltages"""
    adc_status();
    adc_alert_status();

@show.command()
def fans():
    """Display Platform environment fans"""
    fan_status();
    fan_airflow();
    fan_speed();
    fan_alert();

@show.command()
def temps():
    """Display Platform environment temps"""
    sensors_temp();
    sensors_temp_alert();

@show.command()
def smartfan():
    """Display Platform environment smartfan"""
    smartFan_status();

@show.command()
def pse():
    """Display Platform environment pse"""
    pse_status();

@show.command()
def system():
    """Display Platform environment system"""
    system_info();

if __name__ == "__main__":
    cli()
