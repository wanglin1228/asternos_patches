#############################################################################
# Asterfusion
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Chassis information which are available in the platform
#
#############################################################################


try:
    import sys
    import os
    from sonic_platform_base.sonic_sfp.sfputilhelper import SfpUtilHelper
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_py_common import device_info
    sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    from plugins.helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

NUM_FAN_TRAY = 4
NUM_PSU = 2
NUM_THERMAL = 7
NUM_SFP = 56
NUM_COMPONENT = 3

class Chassis(ChassisBase):
    """Platform-specific Chassis class"""

    def __init__(self):
        ChassisBase.__init__(self)
        self._api_helper = APIHelper()
        self.sfp_module_initialized = False
        self.__initialize_eeprom()

        self.__initialize_thermals()
        self.__initialize_components()

        self.sfp_module_initialized = False

    def __initialize_sfp(self):
        sfputil_helper = SfpUtilHelper()
        port_config_file_path = device_info.get_path_to_port_config_file()
        sfputil_helper.read_porttab_mappings(port_config_file_path, 0)

        from plugins.sfputil import SfpUtil
        for index in range(0, NUM_SFP):
            sfp = Sfp(index, sfputil_helper.logical[index])
            self._sfp_list.append(sfp)
        self.sfp_module_initialized = True

    def __initialize_thermals(self):
        from plugins.thermalutil import ThermalUtil
        for index in range(0, NUM_THERMAL):
            thermal = ThermalUtil().get_thermal_info_key(index)
            self._thermal_list.append(thermal)

    def __initialize_components(self):
        from plugins.component import Component
        for index in range(0, NUM_COMPONENT):
            component = Component(index)
            self._component_list.append(component)

    def __initialize_eeprom(self):
        from plugins.eeprom import board
        self._eeprom = board('','','','')

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis
        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        self._eeprom.uart_get("base_mac_address")

    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the chassis
        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their corresponding
            values.
        """
        self._eeprom.get_eeprom()

    ##############################################################
    ######################## SFP methods #########################
    ##############################################################

    def get_num_sfps(self):
        """
        Retrieves the number of sfps available on this chassis
        Returns:
            An integer, the number of sfps available on this chassis
        """
        from plugins.fanutil import FanUtil
        if not self.sfp_module_initialized:
            self.__initialize_sfp()

        return SfpUtil.PORTS_IN_BLOCK

    ##############################################################
    ################## ThermalManager methods ####################
    ##############################################################

    def get_thermal_manager(self):
        from plugins.thermal_manager import ThermalManager
        return ThermalManager

    ##############################################################
    ###################### Device methods ########################
    ##############################################################

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        return self._api_helper.hwsku

    def get_presence(self):
        """
        Retrieves the presence of the Chassis
        Returns:
            bool: True if Chassis is present, False if not
        """
        return True

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        data = self._eeprom.uart_get("serial_number")
        return data.decode() if data else 'N\A'

    def get_model(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        data = self._eeprom.uart_get("product_name")
        return data.decode() if data else 'N\A'

    def get_revision(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        data = self._eeprom.uart_get("device_version")
        return data.decode() if data else 'N\A'
        
    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return True

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return -1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False
