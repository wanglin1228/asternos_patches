#!/usr/bin/env python

try:
    import time
    import json
    import os
    import subprocess
    from sonic_platform_base.sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

attr_path = '/sys/class/hwmon/hwmon1/device/'
PLATFORM_INSTALL_INFO_FILE="/etc/sonic/platform_install.json"
PLATFORM_SFP_GROUPS = ['SFP-G01','SFP-G02','SFP-G03','SFP-G04','SFP-G05', 'SFP-G06', 'SFP-G07']

QSFP_LOWPOWER_OFFSET = 93
QSFP_LOWPOWER_WIDTH = 1
LPMODE_NOT_SUPPORT = 2
LPMODE_SUCCESS = 0
LPMODE_FAILED = 1
LPMODE_ON = 0
LPMODE_OFF = 1

class SfpUtil(SfpUtilBase):
    """Platform specific SfpUtill class"""

    _port_start = 0
    _port_end = 55
    _qsfp_port_start = 48
    _port_in_block =56
    PORTS_IN_BLOCK = 56
    _port_to_eeprom_mapping = {}
    _global_port_pres_dict = {}

    def __init__(self):
        eeprom_path = "{}/eeprom"
        path_list = self.get_sfp_path()
        for x in range(self._port_start, self._port_end + 1):
            port_eeprom_path = eeprom_path.format(path_list[x])
            self._port_to_eeprom_mapping[x] = port_eeprom_path

        self.init_global_port_presence()        
        SfpUtilBase.__init__(self)

    def get_sfp_path(self):
        map = []
        with open(PLATFORM_INSTALL_INFO_FILE) as fd:
            install_info = json.load(fd)
            for sfp_group_name in PLATFORM_SFP_GROUPS:
                sfp_group = install_info[1][sfp_group_name]
                map = map + sfp_group['paths']
            return map

    def get_hwmon_path(self):
        '''Find hwmon path. Return empty string if not found. Return first path if found more than one path.'''
        # limit maxdepth is necessary, or we may get infinite results because of loop symbol link(s)
        # redirect stderr to blackhole is necessary, or we will get something like "File system loop detected"
        cmd = ["find -L /sys/class/hwmon/ -maxdepth 3 -type d -name CX308P_SFP 2>/dev/null"]
        find = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
        output, err = find.communicate()
        if type(output) == bytes:
            output = output.decode()
        output = output.strip()
        if len(output) < 1:
            return ""
        # Here we return the upper path instead of detailed path. e.g. '/sys/class/hwmon/hwmon2/device/'
        return "/".join(output.split("\n")[0].split("/")[:-1]) + "/"

    def reset(self, port_num):
        # Check for invalid port_num
        if port_num < self._qsfp_port_start or port_num > self._port_end:
            return False

        global attr_path
        if not os.path.exists(attr_path+"CX308P_QSFP"):
            attr_path = self.get_hwmon_path()
            if len(attr_path) <= 0:
                print("CX308P_QSFP sysfs directory not found. Please check platform driver.")
                return False
        path = attr_path+'CX308P_QSFP/QSFP_reset'
        try:
            reg_file = open(path, 'w')
        except IOError as e:
            print( "Error: unable to open file: %s" % str(e))
            return False

        #toggle reset
        #reg_file.seek(0)
        reg_file.write(str(port_num-self._qsfp_port_start+1))
        #time.sleep(1)
        #reg_file.seek(0)
        #reg_file.write('0')
        reg_file.close()
        return True

    def set_low_power_mode(self, port_num, lpmode):
        # Check for invalid port_num
        if port_num < self._qsfp_port_start or port_num > self._port_end:
            return LPMODE_NOT_SUPPORT

        file_path = self._port_to_eeprom_mapping[port_num]
        try:
            sysfsfile_eeprom_r = open(file_path, mode="rb", buffering=0)
            sysfsfile_eeprom_w = open(file_path, mode="wb", buffering=0)
        except IOError:
            return LPMODE_FAILED
            
        try:
            sysfsfile_eeprom_r.seek(QSFP_LOWPOWER_OFFSET)
            val = sysfsfile_eeprom_r.read(QSFP_LOWPOWER_WIDTH)
        except IOError:
            sysfsfile_eeprom_r.close()
            return LPMODE_FAILED

        if lpmode is True:
            val = (ord(val) & 0xfd) | 0x2
        else:
            val = (ord(val) & 0xfd)

        sysfsfile_eeprom_w.seek(QSFP_LOWPOWER_OFFSET)
        sysfsfile_eeprom_w.write(bytearray([val]))
        sysfsfile_eeprom_r.close()
        sysfsfile_eeprom_w.close()
        return LPMODE_SUCCESS

    def get_low_power_mode(self, port_num):
        # Check for invalid port_num
        if port_num < self._qsfp_port_start or port_num > self._port_end:
            return LPMODE_NOT_SUPPORT

        file_path = self._port_to_eeprom_mapping[port_num]
        try:
            sysfsfile_eeprom = open(file_path, mode="rb", buffering=0)
        except IOError:
            return LPMODE_FAILED
            
        try:
            sysfsfile_eeprom.seek(QSFP_LOWPOWER_OFFSET)
            val = sysfsfile_eeprom.read(QSFP_LOWPOWER_WIDTH)
        except IOError:
            sysfsfile_eeprom.close()
            return LPMODE_FAILED

        sysfsfile_eeprom.close()
        val = (ord(val) >> 1) & 0x1
        if val == 1:
            return LPMODE_ON
        else:
            return LPMODE_OFF
        
    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self._port_start or port_num > self._port_end:
            return False

        global attr_path
        if not os.path.exists(attr_path+"CX308P_SFP") or not os.path.exists(attr_path+"CX308P_QSFP"):
            attr_path = self.get_hwmon_path()
            if len(attr_path) <= 0:
                print("CX308P_(Q)SFP sysfs directory not found. Please check platform driver.")
                return False
        if port_num >= self._qsfp_port_start:
            path = attr_path+'CX308P_QSFP/QSFP_present'
            line = port_num - self._qsfp_port_start
        else:
            path = attr_path+'CX308P_SFP/SFP_present'
            line = port_num
            
        try:
          reg_file = open(path, 'r')
        except IOError as e:
            print( "Error: unable to open file: %s" % str(e))
            return False
        text_lines = reg_file.readlines()
        reg_file.close()
        if text_lines[line].find('not') < 0:
            return True

        return False

    def init_global_port_presence(self):
        for port_num in range(self.port_start, (self.port_end + 1)):
            presence = self.get_presence(port_num)
            if(presence):
                self._global_port_pres_dict[port_num] = '1'
            else:
                self._global_port_pres_dict[port_num] = '0'  
 
    def get_transceiver_change_event(self, timeout=0):
        port_dict = {}
        start = time.time()
        while True:
            for port_num in range(self.port_start, (self.port_end + 1)):
                presence = self.get_presence(port_num)
                if(presence and self._global_port_pres_dict[port_num] == '0'):
                    self._global_port_pres_dict[port_num] = '1'
                    port_dict[port_num] = '1'
                elif(not presence and
                     self._global_port_pres_dict[port_num] == '1'):
                    self._global_port_pres_dict[port_num] = '0'
                    port_dict[port_num] = '0'

            if(len(port_dict) > 0):
                return True, port_dict
            if timeout == 0  or (timeout > 0 and time.time() - start > timeout / 1000):
                break
            time.sleep(1)
        return False, port_dict

    @property
    def port_start(self):
        return self._port_start

    @property
    def port_end(self):
        return self._port_end

    @property
    def qsfp_ports(self):
        return range(self._qsfp_port_start, self._port_in_block + 1)

    @property 
    def port_to_eeprom_mapping(self):
         return self._port_to_eeprom_mapping
