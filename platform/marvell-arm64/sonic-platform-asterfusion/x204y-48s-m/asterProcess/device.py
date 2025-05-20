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

SFP_MAX_NUM = 52
CPLDA_REGISTER = 70
CPLD_SFP_NUM = 8

DEVICE_BUS = {'cpld': ['1-0070', '1-0071', '1-0072', '1-0073', '1-0074', '1-0075', '1-0076']} # arbiter

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
			led		: Led controls
			locate		: Blink locator LED
			sensors		: Read HW monitors
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


class PlatformStatusThread(common.threading.Thread):
	def __init__(self,threadname, timer):
		self.running = True
		common.threading.Thread.__init__(self,name = threadname)
		self.timer = timer
		self.fan_led_status = 'off'
		self.psu_led_status = 'off'

	def run(self):
		while common.RUN:
			self.checkPlatformStatus()
			common.time.sleep(self.timer)

	def checkPlatformStatus(self):
		total_result = common.PASS
		total_result += self.checkFanStatus()
		total_result += self.checkPsuStatus()

	def checkFanStatus(self):
		val = common.readFile(common.SENSORS_PATH + 'temp1_input')
		tmp = (int(val,10))/1000
		if tmp <= 15:
			fan_level = '0'
		elif tmp <= 20:
			fan_level = '1'
		elif tmp <= 25:
			fan_level = '2'
		elif tmp <= 30:
			fan_level = '3'
		elif tmp <= 35:
			fan_level = '4'
		elif tmp <= 40:
			fan_level = '5'
		elif tmp <= 45:
			fan_level = '6'
		else:
			fan_level = '7'
		result = common.writeFile(common.FAN_PATH + 'fan_level', fan_level)
		return common.PASS
	def checkPsuStatus(self):
		return common.PASS

def deviceInit():

	# Set tx disable
	#cpld_bus = DEVICE_BUS['cpld']
	#for x in range(0, SFP_MAX_NUM):
		#bus = CPLDA_REGISTER+(x/8)

		#path = common.I2C_PREFIX + bus + '/sfp' + str(x+1) + '_tx_disable'
		#result = common.writeFile(path, "0")

	#set led to green
	result = common.writeFile(common.LED_PATH + 'sys_led', '1')

	return
