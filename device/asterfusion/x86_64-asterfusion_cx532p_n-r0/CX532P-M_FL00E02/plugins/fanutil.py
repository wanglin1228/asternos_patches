#!/usr/bin/env python

#############################################################################
# Asterfusion
#
# Module contains an implementation of SONiC FAN Base API and
# provides the FAN status which are available in the platform
#
#############################################################################

import os.path

try:
    from sonic_platform_base.fan_base import FanBase
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

class FanUtil(FanBase):
    """Platform-specific FanUtil class"""

    def __init__(self):
        FanBase.__init__(self)

        self.fan_device_path = "/sys/class/hwmon/hwmon1/device/CX532P_FAN/"
        self.fan_status = 'fan{0}_status'
        self.fan_speed = 'fan{0}_speed_rpm'
        self.fan_presence = 'fan{0}_present'
        self.fan_direction = 'fan{0}_direction'

        self.index_to_fan_mapping = {
            1: 'Fan 1',
            2: 'Fan 2',
            3: 'Fan 3',
            4: 'Fan 4',
            5: 'Fan 5'
        }

    def get_num_fan(self):
        return len(self.index_to_fan_mapping)

    def get_fan_info_dict(self, index):
        if index is None:
            return False

        fan_info_dict = {}
        front_rpm = 'N/A'
        state_node = self.fan_device_path + self.fan_status.format(index)
        presence_node = self.fan_device_path + self.fan_presence.format(index)
        speed_node = self.fan_device_path + self.fan_speed.format(index)
        direction_node = self.fan_device_path + self.fan_direction.format(index)
        try:
            with open(state_node, 'r') as state_file,\
                    open(presence_node, 'r') as presence_file,\
                    open(speed_node, 'r') as speed_file,\
                    open(direction_node, 'r') as direction_file:

                fan_state_data = state_file.read()
                fan_presence_data = presence_file.read()
                fan_speed_data = speed_file.readlines()
                fan_direction_date = direction_file.read()
                
                fan_state_fail = self.index_to_fan_mapping[index] + ' is Fail'
                fan_state_good = self.index_to_fan_mapping[index] + ' is Good'
                fan_presence = self.index_to_fan_mapping[index] + ' is present'
                fan_presence_na = self.index_to_fan_mapping[index] + ' is unknown'
                fan_direction_intake = self.index_to_fan_mapping[index] + ' is intake'
                fan_direction_exhaust = self.index_to_fan_mapping[index] + ' is exhaust'

                fan_info_dict['status'] = 'false' if (fan_state_fail in fan_state_data) else \
                                          'true' if (fan_state_good in fan_state_data) else \
                                          'N/A'
                fan_info_dict['presence'] = 'true' if (fan_presence in fan_presence_data) else\
                                            'N/A' if (fan_presence_na in fan_presence_data) else \
                                            'false'
                fan_info_dict['direction'] = 'exhaust' if (fan_direction_exhaust in fan_direction_date) else\
                                            'intake' if (fan_direction_intake in fan_direction_date) else\
                                            'N\A'

                for line in fan_speed_data:
                    if 'FanModule{0} Front'.format(index) in line:
                        front_rpm = line.split(':')[1].strip()

                fan_info_dict['speed'] = front_rpm

                state_file.close()
                presence_file.close()
                speed_file.close()
        except IOError:
            return False

        return fan_info_dict
