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
import subprocess
try:
    from sonic_platform_base.thermal_base import ThermalBase
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

class ThermalUtil(ThermalBase):
    """Platform-specific ThermalUtil class"""

    def __init__(self):
        ThermalBase.__init__(self)

        self.thermal_virtual_path = "/sys/class/thermal/thermal_zone0/"
        self.thermal_core_path = "/sys/class/hwmon/hwmon1/"
        self.thermal_device_path = "/sys/class/hwmon/hwmon2/device/CX308P_THERMAL/"

        self.thermal_key = 0
        self.thermal_temp = 1
        self.thermal_alarm = 2
        self.thermal_crit = 3
        self.index_to_temp_mapping = {
            0: ['CPU_0', 'temp', 'trip_point_1_temp', 'trip_point_0_temp'],
            1: ['Physical_id_0', 'temp1_input', 'temp1_max', 'temp1_crit'],
            6: ['Thermal_sensor_1', 'thermal_sersor_1'],
            7: ['Thermal_sensor_2', 'thermal_sersor_2'],
            8: ['Thermal_sensor_3', 'thermal_sersor_3'],
            9: ['Thermal_sensor_4', 'thermal_sersor_4']
        }
        core_count = 0
        for i in range(2,20):
            if core_count == 4:
                break
            if not os.path.exists(self.thermal_core_path+'temp{}_input'.format(i)):
                self.thermal_core_path = "/".join(self.get_thermal_hwmon_path("temp{}_input".format(i)).split("/")[:-2]) + "/"
                if len(self.thermal_core_path) <= 2:
                    continue
            self.index_to_temp_mapping[core_count+2] = ['CORE_{}'.format(core_count), 'temp{}_input'.format(i), 'temp{}_max'.format(i), 'temp{}_crit'.format(i)]
            core_count = core_count + 1

    def get_num_thermal(self):
        return len(self.index_to_temp_mapping)

    def get_thermal_info_key(self, index):
        if index is None:
            return False
        return self.index_to_temp_mapping[index][self.thermal_key]

    def get_thermal_hwmon_path(self, keyfile):
        '''Find thermal hwmon path. Return empty string if not found. Return first path if found more than one path.'''
        # limit maxdepth is necessary, or we may get infinite results because of loop symbol link(s)
        # redirect stderr to blackhole is necessary, or we will get something like "File system loop detected"
        cmd = ["find -L /sys/class/hwmon/ -maxdepth 3 -name {} 2>/dev/null".format(keyfile)]
        find = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
        output, err = find.communicate()
        if type(output) == bytes:
            output = output.decode()
        output = output.strip()
        if len(output) < 1:
            return ""
        return output.split("\n")[0] + "/"

    def get_thermal_info_dict(self, index):
        if index is None:
            return dict()

        thermal_info_dict = {}

        if index == 0:
            temp_node = self.thermal_virtual_path + self.index_to_temp_mapping[index][self.thermal_temp]
            alarm_node = self.thermal_virtual_path + self.index_to_temp_mapping[index][self.thermal_alarm]
            crit_node = self.thermal_virtual_path + self.index_to_temp_mapping[index][self.thermal_crit]
        elif index <= 5:
            temp_node = self.thermal_core_path + self.index_to_temp_mapping[index][self.thermal_temp]
            alarm_node = self.thermal_core_path + self.index_to_temp_mapping[index][self.thermal_alarm]
            crit_node = self.thermal_core_path + self.index_to_temp_mapping[index][self.thermal_crit]
        else:
            if not os.path.exists(self.thermal_device_path):
                self.thermal_device_path = self.get_thermal_hwmon_path("CX308P_THERMAL")
                if len(self.thermal_device_path) <= 0:
                    print("CX308P_THERMAL sysfs directory not found. Please check platform driver.")
                    return False
            temp_node = self.thermal_device_path + self.index_to_temp_mapping[index][self.thermal_temp]

        if index <= 5:
            try:
                with open(temp_node, 'r') as temp_file,\
                        open(alarm_node, 'r') as alarm_file,\
                        open(crit_node, 'r') as crit_file:

                    thermal_info_dict['temperature'] = '%.2f' % (int(temp_file.read(),10)/1000)
                    thermal_info_dict['high_threshold'] = '%.2f' % (int(alarm_file.read(),10)/1000)
                    thermal_info_dict['critical_high_threshold'] = '%.2f' % (int(crit_file.read(),10)/1000)

                    thermal_info_dict['low_threshold'] = 'N/A'
                    thermal_info_dict['critical_low_threshold'] = 'N/A'
                    if float(thermal_info_dict['temperature']) >= float(thermal_info_dict['high_threshold']):
                        thermal_info_dict['warning_status'] = 'true'
                    else:
                        thermal_info_dict['warning_status'] = 'false'

                    temp_file.close()
                    alarm_file.close()
                    crit_file.close()
                    thermal_info_dict['key'] = self.index_to_temp_mapping[index][self.thermal_key]
            except IOError as e:
                print("Error: failed to get thermal info. {}".format(str(e)))
                return dict()
        elif index <= 9 :
            try:
                with open(temp_node, 'r') as temp_file:

                    temp = temp_file.read()
                    if len(temp) > 36:  # e.g. len("Sensor 1 temperature is 33 degrees (C)") == 38
                        try:
                            temp = temp.split("is")[1].split("degree")[0].strip()
                            thermal_info_dict['temperature'] = '%.2f' % (int(temp,10))
                        except:
                            thermal_info_dict['temperature'] = "N/A"
                    else:
                        thermal_info_dict['temperature'] = "N/A"
                    thermal_info_dict['high_threshold'] = 'N/A'
                    thermal_info_dict['critical_high_threshold'] = 'N/A'

                    thermal_info_dict['low_threshold'] = 'N/A'
                    thermal_info_dict['critical_low_threshold'] = 'N/A'
                    thermal_info_dict['warning_status'] = 'false'

                    temp_file.close()
                    thermal_info_dict['key'] = self.index_to_temp_mapping[index][self.thermal_key]
            except IOError as e:
                print("Error: failed to get thermal info. {}".format(str(e)))
                return dict()

        return thermal_info_dict

