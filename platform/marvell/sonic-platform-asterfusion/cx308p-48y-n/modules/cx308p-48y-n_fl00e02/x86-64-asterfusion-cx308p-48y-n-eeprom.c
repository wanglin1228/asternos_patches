/* An hwmon driver for Asterfusion CX308P-48Y-N Marvell i2c Module */
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#include "x86-64-asterfusion-cx308p-48y-n.h"
#include "x86-64-asterfusion-cx308p-48y-n-common.h"

ssize_t eeprom_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status[UART_MAX_SIZE] = {0};
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    sprintf(buf, "");
	
    uart_read_cmd(UART_CMD_EEPROM, attr->index, SUBCMD_DEFAULT, 3, status);
	
    return sprintf(buf, "%s", status);
}
