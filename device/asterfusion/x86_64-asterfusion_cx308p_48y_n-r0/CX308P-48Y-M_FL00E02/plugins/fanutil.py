#!/usr/bin/env python

#############################################################################
# Asterfusion
#
# Module contains an implementation of SONiC FAN Base API and
# provides the FAN status which are available in the platform
#
#############################################################################

import os.path
import subprocess
try:
    from sonic_platform_base.fan_base import FanBase
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

class FanUtil(FanBase):
    """Platform-specific FanUtil class"""

    def __init__(self):
        FanBase.__init__(self)

        self.fan_device_path = "/sys/class/hwmon/hwmon3/device/CX308P_FAN/"
        self.fan_status = 'fan_status'
        self.fan_speed = 'fan_speed_rpm'
        self.fan_presence = 'fan_present'
        self.fan_direction = "fan_airflow"

        self.index_to_fan_mapping = {
            1: 'Fan 1',
            2: 'Fan 2',
            3: 'Fan 3',
            4: 'Fan 4'
        }

    def get_num_fan(self):
        return len(self.index_to_fan_mapping)

    def get_fan_hwmon_path(self):
        '''Find fan hwmon path. Return empty string if not found. Return first path if found more than one path.'''
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
        return output.split("\n")[0] + "/"

    def get_fan_info_dict(self, index):
        if index is None:
            return False

        fan_info_dict = {}

        if not os.path.exists(self.fan_device_path):
            self.fan_device_path = self.get_fan_hwmon_path()
            if len(self.fan_device_path) <= 0:
                print("CX308P_FAN sysfs directory not found. Please check platform driver.")
                return False
        state_node = self.fan_device_path + self.fan_status
        presence_node = self.fan_device_path + self.fan_presence
        speed_node = self.fan_device_path + self.fan_speed
        direction_node = self.fan_device_path + self.fan_direction

        try:
            with open(state_node, 'r') as state_file:
                fan_state_data = state_file.read()
                if "{} is unknown".format(index) in fan_state_data:
                    fan_info_dict["status"] = "N/A"
                elif "{} is Good".format(index) in fan_state_data:
                    fan_info_dict["status"] = "true"
                else:
                    fan_info_dict["status"] = "false"
                state_file.close()

            with open(presence_node, 'r') as presence_file:
                fan_presence_data = presence_file.read()
                if "{} is unknown".format(index) in fan_presence_data:
                    fan_info_dict["presence"] = "N/A"
                elif "{} is present".format(index) in fan_presence_data:
                    fan_info_dict["presence"] = "true"
                else:
                    fan_info_dict["presence"] = "false"
                presence_file.close()

            with open(direction_node, 'r') as direction_file:
                fan_direction_data = direction_file.read()
                if "{} airflow is exhaust".format(index) in fan_direction_data:
                    fan_info_dict["direction"] = "exhaust"
                elif "{} airflow is intake".format(index) in fan_direction_data:
                    fan_info_dict["direction"] = "intake"
                else:
                    fan_info_dict["direction"] = "N/A"
                direction_file.close()

            with open(speed_node, 'r') as speed_file:
                fan_speed_data = speed_file.read()
                lines = fan_speed_data.split("\n")
                needed_lines = []
                for line in lines:
                    if "FanModule{}".format(index) in line:
                        needed_lines.append(line)
                front_rpm = needed_lines[0].split(":")[1].split("RPM")[0].strip()
                fan_info_dict["speed"] = front_rpm
                speed_file.close()

        except IOError:
            return False

        return fan_info_dict
