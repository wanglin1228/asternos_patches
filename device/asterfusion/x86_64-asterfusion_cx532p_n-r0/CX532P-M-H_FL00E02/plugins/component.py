#############################################################################
# Asterfusion
#
# Component contains an implementation of SONiC Platform Base API and
# provides the components firmware management function
#
#############################################################################

import os.path
import shutil
import subprocess

try:
    from sonic_platform_base.component_base import ComponentBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

CPLD_ADDR_MAPPING = {
    "CPLD1": "0x40",
    "CPLD2": "0x40"
}
BIOS_VERSION_PATH = "/sys/class/dmi/id/bios_version"
CPLD_VERSION_PATH = "//sys/class/hwmon/hwmon1/device/SYS_INFO/cpld_version"
COMPONENT_NAME_LIST = ["CPLD1", "CPLD2", "BIOS"]
COMPONENT_DES_LIST = ["Used for managing QSFP+ ports (13-32) SFP ports (33-34)",
                      "Used for managing QSFP+ ports (1-12)",
                      "Basic Input/Output System"]


class Component(ComponentBase):
    """Platform-specific Component class"""

    DEVICE_TYPE = "component"

    def __init__(self, component_index):
        ComponentBase.__init__(self)
        self.index = component_index
        self._api_helper = APIHelper()
        self.name = self.get_name()

    def __get_bios_version(self):
        # Retrieves the BIOS firmware version
        try:
            with open(BIOS_VERSION_PATH, 'r') as fd:
                bios_version = fd.read()
                return bios_version.strip()
        except Exception as e:
            return None

    def __get_cpld_version(self, cpld_num):
        global CPLD_VERSION_PATH
        if not os.path.exists(CPLD_VERSION_PATH):
            CPLD_VERSION_PATH = self.get_cpld_hwmon_path()
            if len(CPLD_VERSION_PATH) <= 2:
                return 'N/A'
            else:
                CPLD_VERSION_PATH = CPLD_VERSION_PATH + "cpld_version"
        try:
            with open(CPLD_VERSION_PATH, 'r') as fd:
                cpld_version = fd.readlines()
                for line in cpld_version:
                    if "CPLD{}".format(cpld_num) in line:
                        return line.split()[1]
        except Exception as e:
            return "N/A"

    def get_cpld_hwmon_path(self):
        '''Find thermal hwmon path. Return empty string if not found. Return first path if found more than one path.'''
        # limit maxdepth is necessary, or we may get infinite results because of loop symbol link(s)
        # redirect stderr to blackhole is necessary, or we will get something like "File system loop detected"
        cmd = ["find -L /sys/class/hwmon/ -maxdepth 3 -name SYS_INFO 2>/dev/null"]
        find = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
        output, err = find.communicate()
        if type(output) == bytes:
            output = output.decode()
        output = output.strip()
        if len(output) < 1:
            return ""
        return output.split("\n")[0] + "/"

    def get_name(self):
        """
        Retrieves the name of the component
         Returns:
            A string containing the name of the component
        """
        return COMPONENT_NAME_LIST[self.index]

    def get_description(self):
        """
        Retrieves the description of the component
            Returns:
            A string containing the description of the component
        """
        return COMPONENT_DES_LIST[self.index]

    def get_firmware_version(self):
        """
        Retrieves the firmware version of module
        Returns:
            string: The firmware versions of the module
        """
        fw_version = None

        if self.name == "BIOS":
            fw_version = self.__get_bios_version()
        elif "CPLD1" in self.name:
            fw_version = self.__get_cpld_version(1)
        elif "CPLD2" in self.name:
            fw_version = self.__get_cpld_version(2)

        return fw_version

    def get_available_firmware_version(self, image_path):
        """
        Retrieves the available firmware version of the component
        Note: the firmware version will be read from image
        Args:
            image_path: A string, path to firmware image
        Returns:
            A string containing the available firmware version of the component
        """
        return "N/A"

    def get_firmware_update_notification(self, image_path):
        """
        Retrieves a notification on what should be done in order to complete
        the component firmware update
        Args:
            image_path: A string, path to firmware image
        Returns:
            A string containing the component firmware update notification if required.
            By default 'None' value will be used, which indicates that no actions are required
        """
        return "None"

    def install_firmware(self, image_path):
        """
        Install firmware to module
        Args:
            image_path: A string, path to firmware image
        Returns:
            A boolean, True if install successfully, False if not
        """
        if not os.path.isfile(image_path):
            return False

        if "CPLD" in self.name:
            img_name = os.path.basename(image_path)
            root, ext = os.path.splitext(img_name)
            ext = ".vme" if ext == "" else ext
            new_image_path = os.path.join("/tmp", (root.lower() + ext))
            shutil.copy(image_path, new_image_path)
            install_command = ["ispvm", str(new_image_path)]
        # elif self.name == "BIOS":
        #     install_command = "afulnx_64 %s /p /b /n /x /r" % image_path

        return self._api_helper.run_command(install_command)


    def update_firmware(self, image_path):
        """
        Updates firmware of the component
        This API performs firmware update: it assumes firmware installation and loading in a single call.
        In case platform component requires some extra steps (apart from calling Low Level Utility)
        to load the installed firmware (e.g, reboot, power cycle, etc.) - this will be done automatically by API
        Args:
            image_path: A string, path to firmware image
        Raises:
            RuntimeError: update failed
        """
        return False


    ##############################################################
    ###################### Device methods ########################
    ##############################################################


    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if FAN is present, False if not
        """
        return True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        return 'N/A'

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        return 'N/A'

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return True

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of
        entPhysicalContainedIn is'0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device
            or -1 if cannot determine the position
        """
        return -1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False