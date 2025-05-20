#
# psuutil.py
# Platform-specific PSU status interface for SONiC
#


import os.path

try:
    from sonic_psu.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

attr_path = '/sys/class/hwmon/hwmon3/device/CX532P_PSU/'

class PsuUtil(PsuBase):
    """Platform-specific PSUutil class"""

    def __init__(self):
        PsuBase.__init__(self)
    
    _psu_key_value_pair_mapping = {}      
    # Get sysfs attribute
    def get_attr_value(self, path):
        
        retval = 'ERR'        
        if (not os.path.isfile(path)):
            return retval

        try:
            with open(path, 'r') as fd:
                retval = fd.read()
        except Exception as error:
            logging.error("Unable to open ", path, " file !")

        retval = retval.rstrip('\r\n')
        return retval

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
        attr_file = 'psu_status'       
        status_path = attr_path + attr_file
        try:
            reg_file = open(status_path, 'r')
        except IOError as e:
            print( "Error: unable to open file: %s" % str(e))
            return False
        text = reg_file.read()
        
        if "PSU {} is power unknown".format(index) in text:
            status = None
        elif "PSU {} is power Good".format(index) in text:
            status = 1

        reg_file.close() 

        """
        get psu power info, store in global array
        """
        attr_file ='psu{}_power'.format(index)
        psu_power_path = attr_path + attr_file
        try:
            reg_file = open(psu_power_path, 'r')
        except IOError as e:
            print( "Error: unable to open file: %s" % str(e))
            return False
        text = reg_file.readlines()
        for line in text:
            line_array = line.split('is')
            if len(line_array) == 2:
                psu_pair_key =line_array[0].strip()
                psu_pair_value = line_array[1].strip()
                psu_key = "{}_{}".format(psu_pair_key,index)
                self._psu_key_value_pair_mapping[psu_key] = psu_pair_value

        return status

    def get_psu_presence(self, index):
        """
        Retrieves the presence status of power supply unit (PSU) defined
                by index <index>
        :param index: An integer, index of the PSU of which to query status
        :return: Boolean, True if PSU is plugged, False if not
        """
        status = 0
        attr_file ='psu_present'
        presence_path = attr_path + attr_file
        try:
            reg_file = open(presence_path, 'r')
        except IOError as e:
            print( "Error: unable to open file: %s" % str(e))
            return False
        text = reg_file.read()

        if "PSU {} is unknown".format(index) in text:
            status = None
        elif "PSU {} is present".format(index) in text:
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
        attr_file ='psu_direction'
        direction_path = attr_path + attr_file
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
        attr_file ='psu_warning'
        warning_path = attr_path + attr_file
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
        attr_file ='psu_direction_warning'
        dierction_warning_path = attr_path + attr_file
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
        iin_key = 'PSU '+ str(index) +' IIN' + '_' + str(index)
        if iin_key in self._psu_key_value_pair_mapping:
            ret = self._psu_key_value_pair_mapping[iin_key]
        else:
            attr_file ='psu{}_power'.format(index)
            iin_path = attr_path + attr_file
            try:
                reg_file = open(iin_path, 'r')
            except IOError as e:
                print( "Error: unable to open file: %s" % str(e))
                return False
            text = reg_file.readlines()
            for line in text:
                if ('PSU '+ str(index) +' IIN') in line:
                    ret = line.split('is')[1].strip()
            reg_file.close()

        iin = float(ret)/1000
        return str(iin)

    def get_psu_iout(self, index):
        iout_key = 'PSU '+ str(index) +' IOUT' + '_' + str(index)
        if iout_key in self._psu_key_value_pair_mapping:
            ret = self._psu_key_value_pair_mapping[iout_key]
        else:
            attr_file ='psu{}_power'.format(index)
            iout_path = attr_path + attr_file
            try:
                reg_file = open(iout_path, 'r')
            except IOError as e:
                print( "Error: unable to open file: %s" % str(e))
                return False
            text = reg_file.readlines()
            for line in text:
                if ('PSU '+ str(index) +' IOUT') in line:
                    ret = line.split('is')[1].strip()
            reg_file.close()

        iout = float(ret)/1000
        return str(iout)

    def get_psu_vin(self, index):
        vin_key = 'PSU '+ str(index) +' VIN' + '_' + str(index)
        if vin_key in self._psu_key_value_pair_mapping:
            ret = self._psu_key_value_pair_mapping[vin_key]
        else:
            attr_file ='psu{}_power'.format(index)
            vin_path = attr_path + attr_file
            try:
                reg_file = open(vin_path, 'r')
            except IOError as e:
                print( "Error: unable to open file: %s" % str(e))
                return False
            text = reg_file.readlines()
            for line in text:
                if ('PSU '+ str(index) +' VIN') in line:
                    ret = line.split('is')[1].strip()
            reg_file.close()

        vin = float(ret)/1000
        return str(vin)

    def get_psu_vout(self, index):
        vout_key = 'PSU '+ str(index) +' VOUT' + '_' + str(index)
        if vout_key in self._psu_key_value_pair_mapping:
            ret = self._psu_key_value_pair_mapping[vout_key]
        else:
            attr_file ='psu{}_power'.format(index)
            vout_path = attr_path + attr_file
            try:
                reg_file = open(vout_path, 'r')
            except IOError as e:
                print( "Error: unable to open file: %s" % str(e))
                return False
            text = reg_file.readlines()
            for line in text:
                if ('PSU '+ str(index) +' VOUT') in line:
                    ret = line.split('is')[1].strip()
            reg_file.close()

        vout = float(ret)/1000
        return str(vout)

    def get_psu_pin(self, index):
        pin_key = 'PSU '+ str(index) +' PIN' + '_' + str(index)
        if pin_key in self._psu_key_value_pair_mapping:
            ret = self._psu_key_value_pair_mapping[pin_key]
        else:
            attr_file ='psu{}_power'.format(index)
            pin_path = attr_path + attr_file
            try:
                reg_file = open(pin_path, 'r')
            except IOError as e:
                print( "Error: unable to open file: %s" % str(e))
                return False
            text = reg_file.readlines()
            for line in text:
                if ('PSU '+ str(index) +' PIN') in line:
                    ret = line.split('is')[1].strip()
            reg_file.close()

        pin = float(ret)
        return str(pin)

    def get_psu_pout(self, index):
        pout_key = 'PSU '+ str(index) +' POUT' + '_' + str(index)
        if pout_key in self._psu_key_value_pair_mapping:
            ret = self._psu_key_value_pair_mapping[pout_key]
        else:
            attr_file ='psu{}_power'.format(index)
            pout_path = attr_path + attr_file
            try:
                reg_file = open(pout_path, 'r')
            except IOError as e:
                print( "Error: unable to open file: %s" % str(e))
                return False
            text = reg_file.readlines()
            for line in text:
                if ('PSU '+ str(index) +' POUT') in line:
                    ret = line.split('is')[1].strip()
            reg_file.close()

        pout = float(ret)
        return str(pout)

    def get_psu_temp(self, index):
        temp = 0
        return str(temp)
    """
        pout_key = 'PSU '+ str(index) +' TEMP' + '_' + str(index)
        if pout_key in self._psu_key_value_pair_mapping:
            ret = self._psu_key_value_pair_mapping[pout_key]
        else:
            attr_file ='psu{}_temp'.format(index)
            module_path = attr_path + attr_file
            try:
                reg_file = open(module_path, 'r')
            except IOError as e:
                print( "Error: unable to open file: %s" % str(e))
                return False
            ret = reg_file.read()
            temp = float(ret)

            reg_file.close()

        return str(temp)
    """
