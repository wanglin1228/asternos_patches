#!/usr/bin/env python
import os
import sys
import subprocess
try:
    from sonic_eeprom import eeprom_tlvinfo
    from sonic_py_common.daemon_base import Logger
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

EEPROM_CACHE = "/var/cache/sonic/decode-syseeprom/syseeprom_cache"
EEPROM_SYSFS = "/sys/class/hwmon/hwmon2/device/SYS_EEPROM/"


SYSLOG_IDENTIFIER = 'eeprom'
# Global logger class instance
logger = Logger(SYSLOG_IDENTIFIER)

class board(eeprom_tlvinfo.TlvInfoDecoder):
    _TLV_INFO_MAX_LEN = 256
    def __init__(self, name, path, cpld_root, ro):

        if not os.path.exists(os.path.dirname(EEPROM_CACHE)):
            try:
                os.makedirs(os.path.dirname(EEPROM_CACHE))
            except OSError as e:
                if e.errno != errno.EEXIST:
                    raise

        self.eeprom_path = EEPROM_CACHE
        self.eeprom_key_list = [0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33,0x34,0xFE]
        self.eeprom_keyname_list = ["product_name", "part_number", "serial_number", "base_mac_address", "manufacture_data", "device_version", "lable_revision", "platform_name", "onie_version", "mac_address", "manufacturer", "country_code", "vendor_name", "diag_version", "service_tag", "switch_verdor", "main_board_version", "come_version", "ghc0_board_version","ghc1_board_version", "eeprom_crc32"]

        super(board, self).__init__(self.eeprom_path, 0, '', True)

    def get_eeprom_hwmon_path(self):
        '''Find eeprom hwmon path. Return empty string if not found. Return first path if found more than one path.'''
        # limit maxdepth is necessary, or we may get infinite results because of loop symbol link(s)
        # redirect stderr to blackhole is necessary, or we will get something like "File system loop detected"
        cmd = ["find -L /sys/class/hwmon/ -maxdepth 3 -type d -name SYS_EEPROM 2>/dev/null"]
        find = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
        output, err = find.communicate()
        if type(output) == bytes:
            output = output.decode()
        output = output.strip()
        if len(output) < 1:
            return ""
        return output.split("\n")[0] + "/"

    def uart_get(self, cmd):
        global EEPROM_SYSFS
        if not os.path.exists(EEPROM_SYSFS):
            EEPROM_SYSFS = self.get_eeprom_hwmon_path()
            if len(EEPROM_SYSFS) <= 0:
                logger.log_error("SYS_EEPROM sysfs directory not found. Please check platform driver.")
                return None
        run_cmd = "cat " + EEPROM_SYSFS + cmd
        logger.log_info(run_cmd)
        try:
            output = subprocess.check_output(run_cmd, shell=True)
        except subprocess.CalledProcessError as e:
            return None
        return output

    def init_eeprom(self):
        eeprom_raw = b''
        tlv_length = 0
        for (key, kname) in zip(self.eeprom_key_list,self.eeprom_keyname_list):
            syseeprom_info = b''
            syseeprom_info  = self.uart_get(kname)
            if (not syseeprom_info or syseeprom_info == b"read failed") and key != 0x24:
                syseeprom_info = b"N/A"
                eeprom_raw += bytearray([key, len(syseeprom_info)]) + syseeprom_info
                tlv_length = tlv_length + len(syseeprom_info) + 2
                continue

            if key == 0x24:
                mac_str = b""
                if not syseeprom_info:
                    mac_str = bytearray([0,0,0,0,0,0])
                else:
                    mac_list = syseeprom_info.decode().split(":")
                    for tlv in mac_list:
                        mac_str += bytearray([int(tlv,16)])
                eeprom_raw += bytearray([key, 6]) + mac_str
                tlv_length = tlv_length + 8
            elif key == 0x26:
                eeprom_raw += bytearray([key, 1, int(syseeprom_info)])
                tlv_length = tlv_length + 3
            elif key == 0x2a:
                eeprom_raw += bytearray([key, 2, 0, int(syseeprom_info)])
                tlv_length = tlv_length + 4
            elif key == 0xfe:
                eeprom_raw += bytearray([key, 4, int(syseeprom_info[2:4],16), int(syseeprom_info[4:6],16), int(syseeprom_info[6:8],16), int(syseeprom_info[8:10],16)])
                tlv_length = tlv_length + 6
            else:
                eeprom_raw += bytearray([key, len(syseeprom_info)]) + syseeprom_info
                tlv_length = tlv_length + len(syseeprom_info) + 2

        tlvheader = b"TlvInfo\x00" + bytearray([1, 0, tlv_length])
        eeprom_raw = tlvheader + eeprom_raw
        crc = self.calculate_checksum(eeprom_raw[:-4])
        crc_1 = (crc & 0xff000000) >> 24
        crc_2 = (crc & 0x00ff0000) >> 16
        crc_3 = (crc & 0xff00) >> 8
        crc_4 = crc & 0x00ff
        eeprom_raw = eeprom_raw[:-4] + bytearray([crc_1, crc_2, crc_3, crc_4])
        if eeprom_raw is not None:
            eeprom_cache = open(EEPROM_CACHE, 'wb')
            eeprom_cache.write(eeprom_raw)
            eeprom_cache.close()

        return True
