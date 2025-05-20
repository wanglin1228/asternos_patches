#!/usr/bin/env python

#############################################################################
# Asterfusion
#
# Module contains an implementation of SONiC Thermal Base API and
# provides the Thermal status which are available in the platform
#
#############################################################################

import os.path
import re

try:
    from sonic_platform_base.thermal_base import ThermalBase
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

class ThermalUtil(ThermalBase):
    """Platform-specific ThermalUtil class"""

    def __init__(self):
        ThermalBase.__init__(self)

        self.thermal_virtual_path = "/sys/class/thermal/thermal_zone0/"
        self.thermal_core_path = "/sys/devices/platform/coretemp.0/hwmon/hwmon0/"
        self.thermal_device_path = "/sys/class/hwmon/hwmon1/device/CX532P_THERMAL/"

        self.thermal_key = 0
        self.thermal_temp = 1
        self.thermal_alarm = 2
        self.thermal_crit = 3

        self.index_to_temp_mapping = {
            0: ['Physical_id_0', 'temp1_input', 'temp1_max', 'temp1_crit'],
            3: ['Left_Main_Board', 'cpu_l_temp'],
            4: ['Right_Main_Board', 'cpu_r_temp'],
            5: ['Left_Fan_Board', 'fan_1_temp'],
            6: ['Right_Fan_Board', 'fan_2_temp'],
            7: ['Temp', 'temp'],
        }
        core_count = 0
        for i in range(2,20):
            if core_count == 2:
                break
            if os.path.exists(self.thermal_core_path+'temp{}_input'.format(i)):
                self.index_to_temp_mapping[core_count+1] = ['CORE_{}'.format(core_count), 'temp{}_input'.format(i), 'temp{}_max'.format(i), 'temp{}_crit'.format(i)]
                core_count = core_count + 1

    def get_num_thermal(self):
        return len(self.index_to_temp_mapping)

    def get_thermal_info_key(self, index):
        if index is None:
            return False
        return self.index_to_temp_mapping[index][self.thermal_key]

    def get_thermal_info_dict(self, index):
        if index is None:
            return False

        thermal_info_dict = {}

        if index <= 2:
            temp_node = self.thermal_core_path + self.index_to_temp_mapping[index][self.thermal_temp]
            alarm_node = self.thermal_core_path + self.index_to_temp_mapping[index][self.thermal_alarm]
            crit_node = self.thermal_core_path + self.index_to_temp_mapping[index][self.thermal_crit]
        else:
            temp_node = self.thermal_device_path + self.index_to_temp_mapping[index][self.thermal_temp]


        if index <= 2:
            try:
                with open(temp_node, 'r') as temp_file,\
                        open(alarm_node, 'r') as alarm_file,\
                        open(crit_node, 'r') as crit_file:

                    temp_data = temp_file.read().rstrip('\n')
                    alarm_data = alarm_file.read().rstrip('\n')
                    crit_data = crit_file.read().rstrip('\n')

                    thermal_info_dict['temperature'] = '%.2f' % (int(temp_data,10)/1000) if temp_data.lstrip("-").isdigit() else \
                                                    'N/A'

                    thermal_info_dict['high_threshold'] = '%.2f' % (int(alarm_data,10)/1000) if alarm_data.lstrip("-").isdigit() else \
                                                        'N/A'

                    thermal_info_dict['critical_high_threshold'] = '%.2f' % (int(crit_data,10)/1000) if crit_data.lstrip("-").isdigit() else \
                                                                'N/A'

                    thermal_info_dict['low_threshold'] = 'N/A'
                    thermal_info_dict['critical_low_threshold'] = 'N/A'
                    if thermal_info_dict['temperature'] != 'N/A' and thermal_info_dict['high_threshold'] != 'N/A':
                        if float(thermal_info_dict['temperature']) >= float(thermal_info_dict['high_threshold']):
                            thermal_info_dict['warning_status'] = 'true'
                        else:
                            thermal_info_dict['warning_status'] = 'false'
                    else:
                        thermal_info_dict['warning_status'] = 'N/A'

                    temp_file.close()
                    alarm_file.close()
                    crit_file.close()
                    thermal_info_dict['key'] = self.index_to_temp_mapping[index][self.thermal_key]
            except IOError:
                return False
        elif index <= 7 :
            try:
                with open(temp_node, 'r') as temp_file:

                    temp = temp_file.read()
                    if not temp or not temp.lstrip("-").rstrip("\n").isdigit():
                        thermal_info_dict['temperature'] = 'N/A'
                    else:
                        thermal_info_dict['temperature'] = '%.2f' % (int(temp,10))
                    thermal_info_dict['high_threshold'] = 'N/A'
                    thermal_info_dict['critical_high_threshold'] = 'N/A'

                    thermal_info_dict['low_threshold'] = 'N/A'
                    thermal_info_dict['critical_low_threshold'] = 'N/A'
                    thermal_info_dict['warning_status'] = 'false'

                    temp_file.close()
                    thermal_info_dict['key'] = self.index_to_temp_mapping[index][self.thermal_key]
            except IOError:
                return False

        return thermal_info_dict

