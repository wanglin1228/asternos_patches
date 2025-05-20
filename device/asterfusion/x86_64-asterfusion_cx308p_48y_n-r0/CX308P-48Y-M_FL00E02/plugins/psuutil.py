#
# psuutil.py
# Platform-specific PSU status interface for SONiC
#


import os.path
import subprocess
try:
    from sonic_psu.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

psu_sysfs_path = '/sys/class/hwmon/hwmon3/device/CX308P_PSU/'

class PsuUtil(PsuBase):
    """Platform-specific PSUutil class"""

    def __init__(self):
        PsuBase.__init__(self)
 
    def get_psu_hwmon_path(self):
        '''Find psu hwmon path. Return empty string if not found. Return first path if found more than one path.'''
        # limit maxdepth is necessary, or we may get infinite results because of loop symbol link(s)
        # redirect stderr to blackhole is necessary, or we will get something like "File system loop detected"
        cmd = ["find -L /sys/class/hwmon/ -maxdepth 3 -type d -name CX308P_PSU 2>/dev/null"]
        find = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
        output, err = find.communicate()
        if type(output) == bytes:
            output = output.decode()
        output = output.strip()
        if len(output) < 1:
            return ""
        return output.split("\n")[0] + "/"

    def get_num_psus(self):
        """
        Retrieves the number of PSUs available on the device
        :return: An integer, the number of PSUs available on the device
         """
        MAX_PSUS = 2
        return MAX_PSUS

    def get_psu_status(self, index):
        """
        Retrieves the oprational status of power supply unit (PSU) defined
                by index <index>
        :param index: An integer, index of the PSU of which to query status
        :return: Boolean, True if PSU is operating properly, False if PSU is\
        faulty
        """
        status = 0
        global psu_sysfs_path
        if not os.path.exists(psu_sysfs_path):
            psu_sysfs_path = self.get_psu_hwmon_path()
            if len(psu_sysfs_path) <= 0:
                print("CX308P_PSU sysfs directory not found. Please check platform driver.")
                return False
        attr_file = 'psu_status'
        status_path = psu_sysfs_path + attr_file
        try:
            reg_file = open(status_path, 'r')
        except IOError as e:
            print( "Error: unable to open file: %s" % str(e))
            return False
        text_lines = reg_file.read()

        if "PSU {} is power unknown".format(index) in text_lines:
            status = None
        elif "PSU {} is power Good".format(index) in text_lines:
            status = 1

        reg_file.close()

        return status

    def get_psu_presence(self, index):
        """
        Retrieves the presence status of power supply unit (PSU) defined
                by index <index>
        :param index: An integer, index of the PSU of which to query status
        :return: Boolean, True if PSU is plugged, False if not
        """
        status = 0
        global psu_sysfs_path
        if not os.path.exists(psu_sysfs_path):
            psu_sysfs_path = self.get_psu_hwmon_path()
            if len(psu_sysfs_path) <= 0:
                print("CX308P_PSU sysfs directory not found. Please check platform driver.")
                return False
        attr_file ='psu_present'
        presence_path = psu_sysfs_path + attr_file
        try:
            reg_file = open(presence_path, 'r')
        except IOError as e:
            print( "Error: unable to open file: %s" % str(e))
            return False
        text_lines = reg_file.read()

        if "PSU {} is unknown".format(index) in text_lines:
            status = None
        elif "PSU {} is present".format(index) in text_lines:
            status = 1

        reg_file.close()

        return status

    def get_psu_direction(self, index):
        """
        Retrieves the direction of power supply unit (PSU) defined
                by index <index>
        :param index: An integer, index of the PSU of which to query status
        :return: String
        """
        status = 0
        global psu_sysfs_path
        if not os.path.exists(psu_sysfs_path):
            psu_sysfs_path = self.get_psu_hwmon_path()
            if len(psu_sysfs_path) <= 0:
                print("CX308P_PSU sysfs directory not found. Please check platform driver.")
                return False
        attr_file ='psu_direction'
        direction_path = psu_sysfs_path + attr_file
        try:
            reg_file = open(direction_path, 'r')
        except IOError as e:
            print( "Error: unable to open file: %s" % str(e))
            return False
        text = reg_file.readlines()

        search_str = "PSU {}".format(index) 
        for string in text:
            if search_str in string:
                ret = string.split('is')[1].strip()

        reg_file.close()
    
        return ret

    def get_psu_warning(self, index):
        """
        Retrieves the warning of power supply unit (PSU) defined
                by index <index>
        :param index: An integer, index of the PSU of which to query status
        :return: Boolean, True if PSU is warning, False is not
        """
        status = 0
        global psu_sysfs_path
        if not os.path.exists(psu_sysfs_path):
            psu_sysfs_path = self.get_psu_hwmon_path()
            if len(psu_sysfs_path) <= 0:
                print("CX308P_PSU sysfs directory not found. Please check platform driver.")
                return False
        attr_file ='psu_warning'
        warning_path = psu_sysfs_path + attr_file
        try:
            reg_file = open(warning_path, 'r')
        except IOError as e:
            print( "Error: unable to open file: %s" % str(e))
            return False
        text = reg_file.read()

        search_str = "PSU {} is warning".format(index)
        
        if search_str in text:
            status = 1

        reg_file.close()    
    
        return status

    def get_psu_dierction_warning(self, index):
        """
        Retrieves the dierction warning of power supply unit (PSU) defined
                by index <index>
        :param index: An integer, index of the PSU of which to query status
        :return: Boolean, True if PSU dierction is warning, False is not
        """
        status = 0
        global psu_sysfs_path
        if not os.path.exists(psu_sysfs_path):
            psu_sysfs_path = self.get_psu_hwmon_path()
            if len(psu_sysfs_path) <= 0:
                print("CX308P_PSU sysfs directory not found. Please check platform driver.")
                return False
        attr_file ='psu_direction_warning'
        dierction_warning_path = psu_sysfs_path + attr_file
        try:
            reg_file = open(dierction_warning_path, 'r')
        except IOError as e:
            print( "Error: unable to open file: %s" % str(e))
            return False
        text = reg_file.read()

        search_str = "PSU {} direction is warning".format(index)
        
        if search_str in text:
            status = 1

        reg_file.close()    
    
        return status

    def get_psu_power(self, index):
        return 'N/A'

    def get_psu_iin(self, index):
        global psu_sysfs_path
        if not os.path.exists(psu_sysfs_path):
            psu_sysfs_path = self.get_psu_hwmon_path()
            if len(psu_sysfs_path) <= 0:
                print("CX308P_PSU sysfs directory not found. Please check platform driver.")
                return False
        attr_file ='psu_module_{}'.format(index)
        module_path = psu_sysfs_path + attr_file
        try:
            reg_file = open(module_path, 'r')
        except IOError as e:
            print( "Error: unable to open file: %s" % str(e))
            return False
        text = reg_file.readlines()
        for line in text:
            if ('PSU '+ str(index) +' IIN') in line:
                iin = line.split('is')[1].strip()
        reg_file.close()
        return iin

    def get_psu_iout(self, index):
        global psu_sysfs_path
        if not os.path.exists(psu_sysfs_path):
            psu_sysfs_path = self.get_psu_hwmon_path()
            if len(psu_sysfs_path) <= 0:
                print("CX308P_PSU sysfs directory not found. Please check platform driver.")
                return False
        attr_file ='psu_module_{}'.format(index)
        module_path = psu_sysfs_path + attr_file
        try:
            reg_file = open(module_path, 'r')
        except IOError as e:
            print( "Error: unable to open file: %s" % str(e))
            return False
        text = reg_file.readlines()
        for line in text:
            if ('PSU '+ str(index) +' IOUT') in line:
                iout = line.split('is')[1].strip()
        reg_file.close()
        return iout

    def get_psu_vin(self, index):
        global psu_sysfs_path
        if not os.path.exists(psu_sysfs_path):
            psu_sysfs_path = self.get_psu_hwmon_path()
            if len(psu_sysfs_path) <= 0:
                print("CX308P_PSU sysfs directory not found. Please check platform driver.")
                return False
        attr_file ='psu_module_{}'.format(index)
        module_path = psu_sysfs_path + attr_file
        try:
            reg_file = open(module_path, 'r')
        except IOError as e:
            print( "Error: unable to open file: %s" % str(e))
            return False
        text = reg_file.readlines()
        for line in text:
            if ('PSU '+ str(index) +' VIN') in line:
                vin = line.split('is')[1].strip()
        reg_file.close()
        return vin

    def get_psu_vout(self, index):
        global psu_sysfs_path
        if not os.path.exists(psu_sysfs_path):
            psu_sysfs_path = self.get_psu_hwmon_path()
            if len(psu_sysfs_path) <= 0:
                print("CX308P_PSU sysfs directory not found. Please check platform driver.")
                return False
        attr_file ='psu_module_{}'.format(index)
        module_path = psu_sysfs_path + attr_file
        try:
            reg_file = open(module_path, 'r')
        except IOError as e:
            print( "Error: unable to open file: %s" % str(e))
            return False
        text = reg_file.readlines()
        for line in text:
            if ('PSU '+ str(index) +' VOUT') in line:
                vout = line.split('is')[1].strip()
        reg_file.close()
        return vout

    def get_psu_pin(self, index):
        global psu_sysfs_path
        if not os.path.exists(psu_sysfs_path):
            psu_sysfs_path = self.get_psu_hwmon_path()
            if len(psu_sysfs_path) <= 0:
                print("CX308P_PSU sysfs directory not found. Please check platform driver.")
                return False
        attr_file ='psu_module_{}'.format(index)
        module_path = psu_sysfs_path + attr_file
        try:
            reg_file = open(module_path, 'r')
        except IOError as e:
            print( "Error: unable to open file: %s" % str(e))
            return False
        text = reg_file.readlines()
        for line in text:
            if ('PSU '+ str(index) +' PIN') in line:
                pin = line.split('is')[1].strip()
        reg_file.close()
        return pin

    def get_psu_pout(self, index):
        global psu_sysfs_path
        if not os.path.exists(psu_sysfs_path):
            psu_sysfs_path = self.get_psu_hwmon_path()
            if len(psu_sysfs_path) <= 0:
                print("CX308P_PSU sysfs directory not found. Please check platform driver.")
                return False
        attr_file ='psu_module_{}'.format(index)
        module_path = psu_sysfs_path + attr_file
        try:
            reg_file = open(module_path, 'r')
        except IOError as e:
            print( "Error: unable to open file: %s" % str(e))
            return False
        text = reg_file.readlines()
        for line in text:
            if ('PSU '+ str(index) +' POUT') in line:
                pout = line.split('is')[1].strip()
        reg_file.close()
        return pout

    def get_psu_temp(self, index):
        return 'N/A'