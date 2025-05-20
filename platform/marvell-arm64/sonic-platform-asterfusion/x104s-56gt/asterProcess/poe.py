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
            cmd = "API_BT_Share_workspace 0x02 0x82 0X05 0XC5 0X%02X 0X4E 0X4E 0X4E 0X4E 0X4E 0X4E 0X4E 0X4E" % index
            try:
                ret = os.popen(cmd)
            except Exception as error:
                logging.error("Unable to exec ", cmd, " !")
                return False

            read_back = ret.readlines()[2].split()[2:]   #'read back: 0x52 0xdf 0x01 0x00 0x4e 0x4e 0x4e 0x4e 0x4e 0x4e 0x4e 0x4e 0x4e 0x03 0xf0 \n'
            if len(read_back) ==0 or len(read_back) < 13:
                current = voltage = power = temp = max_power = 'N/A'
            else:
                current_value = ((int(read_back[4],16) << 8) + int(read_back[5],16)) * 0.001
                voltage_value = ((int(read_back[9],16) << 8) + int(read_back[10],16)) * 0.1
                power = '%.3f' % (current_value * voltage_value)
                current = '%.4f' % current_value
                voltage = '%.3f' % voltage_value
                temp = 'N/A'
                max_power = '30'
        else:
            current = voltage = power = temp = max_power = 'N/A'

        state_db.set(state_db.STATE_DB, 'POE_INFO|{}'.format(interface_name), POE_INFO_CURRENT_FIELD, current)
        state_db.set(state_db.STATE_DB, 'POE_INFO|{}'.format(interface_name), POE_INFO_VOLTAGE_FIELD, voltage)
        state_db.set(state_db.STATE_DB, 'POE_INFO|{}'.format(interface_name), POE_INFO_POWER_FIELD, power)
        state_db.set(state_db.STATE_DB, 'POE_INFO|{}'.format(interface_name), POE_INFO_TEMP_FIELD, temp)
        state_db.set(state_db.STATE_DB, 'POE_INFO|{}'.format(interface_name), POE_INFO_MAX_POWER_FIELD, max_power)
