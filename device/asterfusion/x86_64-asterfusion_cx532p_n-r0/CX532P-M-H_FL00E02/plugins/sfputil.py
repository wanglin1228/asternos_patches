#!/usr/bin/env python

try:
    import time
    import json
    import os
    from sonic_platform_base.sonic_sfp.sfputilbase import SfpUtilBase

    import subprocess
    from swsssdk import SonicV2Connector
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

attr_path = '/sys/class/hwmon/hwmon1/device/'
PLATFORM_INSTALL_INFO_FILE="/etc/sonic/platform_install.json"
PLATFORM_SFP_GROUPS = ['SFP-G01','SFP-G02','SFP-G03','SFP-G04','SFP-AUX']

QSFP_RESET_FILE = 'CX532P_QSFP/qsfp{}_reset'
QSFP_PRESENT_FILE = 'CX532P_QSFP/qsfp{}_present'

SFP_PRESENT_FILE = 'CX532P_QSFP/sfp{}_present'
SFP_TXDIS_FILE = 'CX532P_QSFP/sfp{}_tx_disable'
XPORT_LED_MODE_FILE = 'CX532P_QSFP/xport_led_mode'

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
    _port_end = 33
    _port_qsfp_start = 0
    _port_qsfp_end = 31
    _port_in_block =31
    PORTS_IN_BLOCK = 34
    _port_to_eeprom_mapping = {}
    _global_port_pres_dict = {}

    def __init__(self):
        eeprom_path = "{}/eeprom"
        path_list = self.get_sfp_path()
        for x in range(self._port_start, self._port_end + 1):
            port_eeprom_path = eeprom_path.format(path_list[x])
            self._port_to_eeprom_mapping[x] = port_eeprom_path

        # Get X1 and X2 oper status
        self.db = SonicV2Connector(host="127.0.0.1")
        self.db.connect(self.db.APPL_DB)
        self.db.connect(self.db.COUNTERS_DB)
        # If the LED has been operated on, i2c will no longer be read or written
        self.status_op = 0
        self.prev_in_octets = 0
        self.prev_out_octets = 0

        self.init_global_port_presence()        
        SfpUtilBase.__init__(self)

    def get_sfp_path(self):
        map = []
        with open(PLATFORM_INSTALL_INFO_FILE) as fd:
            install_info = json.load(fd)
            for sfp_group_name in PLATFORM_SFP_GROUPS:
                sfp_group = install_info[2][sfp_group_name]
                map = map + sfp_group['paths']
            return map
            
    def reset(self, port_num):
        # Check for invalid port_num
        if port_num < self._port_qsfp_start or port_num > self._port_qsfp_end:
            return False

        path = attr_path+QSFP_RESET_FILE.format(port_num+1)
        try:
            reg_file = open(path, 'w')
        except IOError as e:
            print( "Error: unable to open file: %s" % str(e))
            return False
        
        reg_file.seek(0)
        reg_file.write('1')

        reg_file.close()
        return True

    def set_low_power_mode(self, port_num, lpmode):
        # Check for invalid port_num
        if port_num < self._port_qsfp_start or port_num > self._port_qsfp_end:
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
        if port_num < self._port_qsfp_start or port_num > self._port_qsfp_end:
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

        if port_num >= self._port_qsfp_start and port_num <= self._port_qsfp_end:
            path = attr_path+QSFP_PRESENT_FILE.format(port_num+1)
        else:
            path = attr_path+SFP_PRESENT_FILE.format(port_num-self._port_qsfp_end)
        try:
            reg_file = open(path, 'r')
        except IOError as e:
            print( "Error: unable to open file: %s" % str(e))
            return False
        text = reg_file.read()
        reg_file.close()
        if int(text) == 1:
            return True

        return False

    def set_x1_x2_led_mode(self, mode):
        path = attr_path+XPORT_LED_MODE_FILE
        try:
            reg_file = open(path, 'w')
        except IOError as e:
            print( "Error: unable to open file: %s" % str(e))
            return False
        reg_file.seek(0)
        reg_file.write(mode)

        reg_file.close()

        return True

    def update_x1_x2_led_status(self, port_num):
        sfp_num = port_num - (self._port_qsfp_end + 1)
        try:
            oper_status = self.db.get(self.db.APPL_DB, 'PORT_TABLE:Ethernet{}'.format(128 + sfp_num), 'oper_status')
            interface_oid = self.db.get(self.db.COUNTERS_DB, "COUNTERS_PORT_NAME_MAP", 'Ethernet{}'.format(128 + sfp_num))
            in_octets = int(self.db.get(self.db.COUNTERS_DB, "COUNTERS:{}".format(interface_oid), "SAI_PORT_STAT_IF_IN_OCTETS"))
            out_octets = int(self.db.get(self.db.COUNTERS_DB, "COUNTERS:{}".format(interface_oid), "SAI_PORT_STAT_IF_OUT_OCTETS"))
            interface_rate = in_octets + out_octets - self.prev_in_octets - self.prev_out_octets
            self.prev_in_octets = in_octets
            self.prev_out_octets = out_octets
        except Exception as e:
            return
        
        read_cmd = "i2cget -f -y 2 0x40 0x19"
        write_cmd = "i2cset -f -y 2 0x40 0x19 {}"
        if sfp_num == 0:
            index = 0xf0
        else:
            index = 0xf
        if oper_status == "up":
            if interface_rate != 0 and ((self.status_op >> (sfp_num*4)) & 0xf) != 3:
                led_status = int(subprocess.check_output(read_cmd, shell=True).strip().decode(), 0)
                subprocess.check_output(write_cmd.format(str(2 << (sfp_num*4) | (index & led_status))), shell=True)
                self.status_op = ((self.status_op & index) | (3 << (sfp_num*4)))
            elif interface_rate == 0 and  ((self.status_op >> (sfp_num*4)) & 0xf) != 1:
                led_status = int(subprocess.check_output(read_cmd, shell=True).strip().decode(), 0)
                subprocess.check_output(write_cmd.format(str(1 << (sfp_num*4) | (index & led_status))), shell=True)
                self.status_op = ((self.status_op & index) | (1 << (sfp_num*4)))
        else:
            if ((self.status_op >> (sfp_num*4)) & 0xf) != 2:
                led_status = int(subprocess.check_output(read_cmd, shell=True).strip().decode(), 0)
                subprocess.check_output(write_cmd.format(str(index & led_status)), shell=True)
                self.status_op = ((self.status_op & index) | (2 << (sfp_num*4)))

    def init_global_port_presence(self):
        for port_num in range(self.port_start, (self.port_end + 1)):
            presence = self.get_presence(port_num)
            if(presence):
                self._global_port_pres_dict[port_num] = '1'
            else:
                self._global_port_pres_dict[port_num] = '0'
            # changed to hardware led control
            # if port_num in range((self._port_qsfp_end + 1), (self.port_end + 1)):
            #    self.update_x1_x2_led_status(port_num)
        self.set_x1_x2_led_mode('1')
 
    def get_transceiver_change_event(self, timeout=0):
        port_dict = {}
        start = time.time()
        while True:
            for port_num in range(self.port_start, (self.port_end + 1)):
                # if port_num in range((self._port_qsfp_end + 1), (self.port_end + 1)):
                #    self.update_x1_x2_led_status(port_num)
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
        return True, port_dict

    @property
    def port_start(self):
        return self._port_start

    @property
    def port_end(self):
        return self._port_end

    @property
    def qsfp_ports(self):
        return range(0, self._port_in_block + 1)

    @property 
    def port_to_eeprom_mapping(self):
         return self._port_to_eeprom_mapping
    
    def _convert_string_to_num(self, value_str):
        if "-inf" in value_str:
            return 'N/A'
        elif "Unknown" in value_str:
            return 'N/A'
        elif 'dBm' in value_str:
            t_str = value_str.rstrip('dBm')
            return float(t_str)
        elif 'mA' in value_str:
            t_str = value_str.rstrip('mA')
            return float(t_str)
        elif 'C' in value_str:
            t_str = value_str.rstrip('C')
            return float(t_str)
        elif 'Volts' in value_str:
            t_str = value_str.rstrip('Volts')
            return float(t_str)
        else:
            return 'N/A'
