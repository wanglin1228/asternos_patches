#!/usr/bin/env python

#############################################################################
# Asterfusion
#
# Module contains Voltage states which are available in the platform
#
#############################################################################

import os.path
import math

class VoltageUtil(object):
    """Platform-specific FanUtil class"""

    def __init__(self):
        self.voltage_device_path = "/sys/bus/i2c/devices/1-004e/"
        self.index_to_voltage_mapping = {
            0: ['CORE_VOLTAGE', 'core_voltage_high', 'core_voltage_low']
        }

    def get_num_voltage(self):
        return len(self.index_to_voltage_mapping)

    def get_voltage_info_key(self, index):
        if index is None:
            return False
        return self.index_to_voltage_mapping[index][0]

    def get_voltage_info_dict(self, index):

        voltage_info_dict = {}
        try:
                voltage_info_dict['core_voltage'] = 'N/A'
                voltage_info_dict['key'] = self.index_to_voltage_mapping[index][0]
        except IOError:
            return False

        return voltage_info_dict

