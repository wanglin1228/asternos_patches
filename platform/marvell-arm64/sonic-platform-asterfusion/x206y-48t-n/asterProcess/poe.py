# -*- coding: utf-8 -*-
#!/usr/sbin/env python3

import os
import logging
from swsscommon.swsscommon import ConfigDBConnector, SonicV2Connector, SonicDBConfig


POE_INFO_CURRENT_FIELD = 'current(A)'
POE_INFO_POWER_FIELD = 'power(W)'
POE_INFO_VOLTAGE_FIELD = 'voltage(V)'
POE_INFO_TEMP_FIELD = 'temperature(C)'
POE_INFO_MAX_POWER_FIELD = "max_power(W)"

def _wrapper_get_poe_position(index):
    addrIndex = index/4
    if addrIndex==0:
        return '1-0022'
    elif addrIndex==1:
        return '1-0023'
    elif addrIndex==2:
        return '1-0024'
    elif addrIndex==3:
        return '1-0025'
    elif addrIndex==4:
        return '1-0026'
    elif addrIndex==5:
        return '1-0027'
    elif addrIndex==6:
        return '1-0028'
    elif addrIndex==7:
        return '1-0029'
    elif addrIndex==8:
        return '1-002c'
    elif addrIndex==9:
        return '1-002d'
    elif addrIndex==10:
        return '1-0030'
    elif addrIndex==11:
        return '1-0031'
    else:
        return 'NOT-FOUND'

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


def _wrapper_get_poe_current(index):
    poe_position = _wrapper_get_poe_position(index)
    port_index = index % 4 + 1
    if poe_position == 'NOT-FOUND':
        return 'N/A'
    else:
        poe_current  = get_attr_value('/sys/bus/i2c/devices/' + poe_position + '/pse_port{}_current'.format(port_index))
        if poe_current != 'ERR':
            return (int(poe_current, 16) * 61.035 / 1000000.0)
        else:
            return 'N/A'

def _wrapper_get_poe_voltage(index):
    poe_position = _wrapper_get_poe_position(index)
    port_index = index % 4 + 1
    if poe_position == 'NOT-FOUND':
        return 'N/A'
    else:
        poe_voltage = get_attr_value('/sys/bus/i2c/devices/' + poe_position + '/pse_port{}_voltage'.format(port_index))
        if poe_voltage != 'ERR':
            return (int(poe_voltage, 16) * 3.662 / 1000.0)
        else:
            return 'N/A'

def _wrapper_get_poe_temp(index):
    poe_position = _wrapper_get_poe_position(index)
    if poe_position == 'NOT-FOUND':
        return 'N/A'
    else:
        poe_temp  = get_attr_value('/sys/bus/i2c/devices/' + poe_position + '/pse_temperature')
        if poe_temp != 'ERR':
            return (int(poe_temp, 16) * 0.652 - 20.0)
        else:
            return 'N/A'

def poe_db_update():
    config_db = ConfigDBConnector()
    try:
        config_db.connect()
    except Exception as error:
        return False

    port_dict = config_db.get_table('PORT')
    if not port_dict:
        return False

    state_db = SonicV2Connector(host="127.0.0.1")
    try:
        state_db.connect(state_db.STATE_DB)
    except Exception as error:
        return False

    for interface_name in port_dict.keys():
        try:
            media_type = port_dict[interface_name]['media_type']
        except KeyError:
            media_type = 'Unknown'
        try:
            poe_status = port_dict[interface_name]['poe_status']
        except KeyError:
            poe_status = 'Unknown'
        
        if media_type == 'copper' and poe_status == 'enable':
            index = int(port_dict[interface_name]['index'])
            current_value = _wrapper_get_poe_current(index)
            voltage_value = _wrapper_get_poe_voltage(index)
            power = '%.3f' % (current_value * voltage_value)
            current = '%.4f' % current_value
            voltage = '%.3f' % voltage_value
            temp = '%.3f' % _wrapper_get_poe_temp(index)
            max_power = '30'
        else:
            current = voltage = power = temp = max_power = 'N/A'

        state_db.set(state_db.STATE_DB, 'POE_INFO|{}'.format(interface_name), POE_INFO_CURRENT_FIELD, current)
        state_db.set(state_db.STATE_DB, 'POE_INFO|{}'.format(interface_name), POE_INFO_VOLTAGE_FIELD, voltage)
        state_db.set(state_db.STATE_DB, 'POE_INFO|{}'.format(interface_name), POE_INFO_POWER_FIELD, power)
        state_db.set(state_db.STATE_DB, 'POE_INFO|{}'.format(interface_name), POE_INFO_TEMP_FIELD, temp)
        state_db.set(state_db.STATE_DB, 'POE_INFO|{}'.format(interface_name), POE_INFO_MAX_POWER_FIELD, max_power)
