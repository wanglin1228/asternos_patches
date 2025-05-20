#!/usr/bin/env python
import os
import sys
import errno
import subprocess
#from sonic_daemon_base.daemon_base import Lock
try:
    from sonic_eeprom import eeprom_tlvinfo
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

EEPROM_CACHE = "/var/cache/sonic/decode-syseeprom/syseeprom_cache"
EEPROM_SYSFS = "/sys/class/hwmon/hwmon1/device/SYS_EEPROM/"

class board(eeprom_tlvinfo.TlvInfoDecoder):
    _TLV_INFO_MAX_LEN = 256
    def __init__(self, name, path, cpld_root, ro):
        self.eeprom_path = EEPROM_CACHE
        super(board, self).__init__(self.eeprom_path, 0, '', True)

    def uart_cache_update(self):
        if not os.path.exists(EEPROM_CACHE):
            try:
                os.makedirs(os.path.dirname(EEPROM_CACHE))
            except OSError as e:
                if e.errno != errno.EEXIST:
                    raise
        elif os.path.getsize(EEPROM_CACHE):
            return True

        self.eeprom_key_list = [0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33,0x34,0xFE]
        self.eeprom_keyname_list = ["product_name", "part_number", "serial_number", "base_mac_address", "manufacture_data", "device_version", "lable_revision", "platform_name", "onie_version", "mac_address", "manufacturer", "country_code", "vendor_name", "diag_version", "service_tag", "switch_verdor", "main_board_version", "come_version", "ghc0_board_version","ghc1_board_version", "eeprom_crc32"]

        eeprom_raw = b""
        tlv_length = 0
        for (key, kname) in zip(self.eeprom_key_list,self.eeprom_keyname_list):
            syseeprom_info = b''
            syseeprom_info = self.uart_get(kname)
            if not syseeprom_info and key != 0x24:
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
            elif key == 0x2A:
                eeprom_raw += bytearray([key, 2, 0, int(syseeprom_info)])
                tlv_length = tlv_length + 4
            elif key == 0xFE:
                eeprom_raw += bytearray([key, 4, int(syseeprom_info[2:4],16), int(syseeprom_info[4:6],16), int(syseeprom_info[6:8],16), int(syseeprom_info[8:10],16)])
                tlv_length = tlv_length + 6
            else:
                eeprom_raw += bytearray([key, len(syseeprom_info)]) + syseeprom_info
                tlv_length = tlv_length + len(syseeprom_info) + 2


        tlvheader = b"TlvInfo\x00" + bytearray([1, 0, tlv_length])
        eeprom_with_header_nocrc = tlvheader + eeprom_raw
        crc = self.calculate_checksum(eeprom_with_header_nocrc[:-4])
        crc_1 = (crc & 0xff000000) >> 24
        crc_2 = (crc & 0x00ff0000) >> 16
        crc_3 = (crc & 0xff00) >> 8
        crc_4 = crc & 0x00ff
        eeprom_with_header_and_crc = eeprom_with_header_nocrc[:-4] + bytearray([crc_1, crc_2, crc_3, crc_4])
        if eeprom_with_header_and_crc is not None:
            eeprom_cache = open(EEPROM_CACHE, 'wb')
            eeprom_cache.write(eeprom_with_header_and_crc)
            eeprom_cache.close()

    def uart_get(self, cmd):
        run_cmd = "cat " + EEPROM_SYSFS + cmd
        output = subprocess.check_output(run_cmd, shell=True)
        if len(output):
            return output
        else:
            return None
