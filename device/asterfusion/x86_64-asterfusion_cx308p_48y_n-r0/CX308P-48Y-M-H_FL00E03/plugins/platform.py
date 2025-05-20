#############################################################################
# Asterfusion
#
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
#
#############################################################################

try:
    import os
    import sys
    import imp
    import subprocess
    from sonic_platform_base.platform_base import PlatformBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

PLATFORM_SPECIFIC_MODULE_NAME = "chassis"
PLATFORM_SPECIFIC_CLASS_NAME = "Chassis"

PLATFORM_ROOT_PATH = '/usr/share/sonic/device'
SONIC_CFGGEN_PATH = '/usr/local/bin/sonic-cfggen'
HWSKU_KEY = 'DEVICE_METADATA.localhost.hwsku'
PLATFORM_KEY = 'DEVICE_METADATA.localhost.platform'

class Platform(PlatformBase):
    """Platform-specific Platform class"""

    def __init__(self):
        PlatformBase.__init__(self)
        self._chassis = self.load_platform_chassis()

    # Returns platform and HW SKU
    def get_platform_and_hwsku(self):
        try:
            proc = subprocess.Popen([SONIC_CFGGEN_PATH, '-H', '-v', PLATFORM_KEY],
                                    stdout=subprocess.PIPE,
                                    shell=False,
                                    stderr=subprocess.STDOUT)
            stdout = proc.communicate()[0]
            proc.wait()
            platform = (stdout.decode()).rstrip('\n')

            proc = subprocess.Popen([SONIC_CFGGEN_PATH, '-d', '-v', HWSKU_KEY],
                                    stdout=subprocess.PIPE,
                                    shell=False,
                                    stderr=subprocess.STDOUT)
            stdout = proc.communicate()[0]
            proc.wait()
            hwsku = (stdout.decode()).rstrip('\n')
        except OSError as e:
            raise OSError("Cannot detect platform")

        return (platform, hwsku)

    # Instantiate SfpUtilHelper class
    def load_platform_chassis(self):
        # Load platform module from source
        (platform, hwsku) = self.get_platform_and_hwsku()
        platform_path = "/".join([PLATFORM_ROOT_PATH, platform])
        hwsku_path = "/".join([platform_path, hwsku])
    
        # we have to make use of sfputil for some features
        # even though when new platform api is used for all vendors.
        # in this sense, we treat it as a part of new platform api.
        # we have already moved sfputil to sonic_platform_base
        # which is the root of new platform api.
        try:
            module_file = "/".join([platform_path, "plugins", PLATFORM_SPECIFIC_MODULE_NAME + ".py"])
            module = imp.load_source(PLATFORM_SPECIFIC_MODULE_NAME, module_file)
        except IOError as e:
            raise IOError("Failed to load platform module '%s': %s" % (PLATFORM_SPECIFIC_MODULE_NAME, str(e)), True)
    
        try:
            platform_chassis_class = getattr(module, PLATFORM_SPECIFIC_CLASS_NAME)
            platform_chassis = platform_chassis_class()
        except AttributeError as e:
            raise AttributeError("Failed to instantiate '%s' class: %s" % (PLATFORM_SPECIFIC_CLASS_NAME, str(e)), True)
    
        return platform_chassis
