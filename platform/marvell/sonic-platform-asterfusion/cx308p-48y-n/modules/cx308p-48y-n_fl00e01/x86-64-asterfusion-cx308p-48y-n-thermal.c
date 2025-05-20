/* An hwmon driver for Asterfusion CX308P-48Y-N Marvell i2c Module */
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#include "x86-64-asterfusion-cx308p-48y-n.h"
#include "x86-64-asterfusion-cx308p-48y-n-common.h"

extern struct i2c_client *CX_308P_i2c_client;

ssize_t thermal_get(struct device *dev, struct device_attribute *da, char *buf)
{
	u8 thermal_sersor_id = 0;
    u8 status[UART_MAX_SIZE] = {0};
    u8 data[UART_MAX_SIZE] = {0};
    int ret = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

	ret = uart_read_cmd(UART_CMD_TEMP, SUBCMD_DEFAULT, SUBCMD_DEFAULT, 3, status);
	switch(attr->index) {
		case THERMAL_SERSOR_1:
			thermal_sersor_id = 1;
			break;
		case THERMAL_SERSOR_2:
			thermal_sersor_id = 2;
			break;
		case THERMAL_SERSOR_3:
			thermal_sersor_id = 3;
			break;
		case THERMAL_SERSOR_4:
			thermal_sersor_id = 4;
			break;
		default:
			thermal_sersor_id = attr->index;
			break;
	}
    debug_print((KERN_DEBUG "DEBUG : THERMAL byte status = 0x%x\n", status[thermal_sersor_id]));

    if(ret < 0 || status[thermal_sersor_id] == 0xfffffffa || status[thermal_sersor_id] == 0xff || status[thermal_sersor_id] == 0xffff)
    {
        sprintf(data, "Access THERMAL module FAILED\n");
    }
    else
    {
          sprintf(data, "Sensor %d temperature is %s%d degrees (C)\n", thermal_sersor_id,(status[thermal_sersor_id] & 0x80)!=0 ? "-":"",read_8bit_temp((status[thermal_sersor_id] & 0x80),status[thermal_sersor_id]));
    }
    return sprintf(buf, "%s", data);
}
