#!/usr/bin/env python3
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import common

class DeviceThread(common.threading.Thread):
    def __init__(self,threadname, q):
        common.threading.Thread.__init__(self,name = threadname)
        self.queue = q

    def run(self):
        while common.RUN:
            message = self.queue.get()
            self.onMessage(message)

    def onMessage(self, message):
        """
        Commands:
            led     : Led controls
            locate      : Blink locator LED
            sensors     : Read HW monitors
        """

        if len(message.command) < 1:
            result = self.onMessage.__doc__
        else:
            if message.command[0] == 'init':
                result = deviceInit()
            else:
                result = self.onMessage.__doc__

        if (message.callback is not None):
            message.callback(result)

def deviceInit():
    #Set tx disable
    # for x in range(0, common.SFP_MAX_NUM):
    #     path = common.I2C_PREFIX + common.SFP_PATH  + 'SFP_tx_ctrl_' + str(x)
    #     result = common.writeFile(path, "0")

    #Set led to green
    result = common.writeFile(common.I2C_PREFIX + common.LED_PATH + 'led_sys', "1")

    #Set rsmu CLK0 and CLK1
    status, output = common.doBash("rsmu_ctl /dev/rsmu0 wr 2010c1b0 8 0x40 0x86 0x58 0xcd 0x04 0x00 0x00 0x02")
    status, output = common.doBash("rsmu_ctl /dev/rsmu0 wr 2010c1bd 1 0x81")
    status, output = common.doBash("rsmu_ctl /dev/rsmu0 wr 2010c1c0 8 0x40 0x81 0xde 0xf5 0x00 0x00 0x80 0x00")
    status, output = common.doBash("rsmu_ctl /dev/rsmu0 wr 2010c1cd 1 0x81")
    status, output = common.doBash("rsmu_ctl /dev/rsmu0 wr 2010c1fd 1 0xc2")
    status, output = common.doBash("rsmu_ctl /dev/rsmu0 wr 2010c2eb 1 0x03")
    status, output = common.doBash("rsmu_ctl /dev/rsmu0 wr 2010c2f7 1 0x03")
    status, output = common.doBash("rsmu_ctl /dev/rsmu0 wr 2010c2fd 1 0xc0")
    return

class PlatformStatusThread(common.threading.Thread):
    def __init__(self,threadname, timer):
        self.running = True
        common.threading.Thread.__init__(self, name = threadname)
        self.timer = timer
        self.fan_led_status = 'off'
        self.psu_led_status = 'off'

    def run(self):
        while common.RUN:
            self.checkPlatformStatus()
            common.time.sleep(self.timer)

    def checkPlatformStatus(self):
        total_result = common.PASS
        total_result += self.checkTempStatus()
        total_result += self.checkFanStatus()
        total_result += self.checkPsuStatus()

    def checkTempStatus(self):
        return common.PASS

    def checkFanStatus(self):
        return common.PASS

    def checkPsuStatus(self):
        return common.PASS
