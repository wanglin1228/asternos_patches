/*
 *
 * asterfusion eeprom device dirver
 * Version v1.2
 *
 * Copyright (C) 2019 Tengfei <tengfei@asterfusion.com>
 * Copyright (C) 2019 Wangzhui <wangzhui@asterfusion.com>
 *
 *
 */
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#include <linux/types.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/dmi.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/i2c-dev.h>
#include "x86-64-asterfusion-cx532p-n.h"
#include "x86-64-asterfusion-cx532p-n-common.h"


ssize_t cx532p_bmc_eeprom_value_return(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct sensor_device_attribute *da = to_sensor_dev_attr(attr);
    union i2c_smbus_data cx532p_eeprom_read_tlv_data = {.block={0x00}};

    get_bmc_data(UART_CMD_EEPROM, da->index, 0xaa, 3, &cx532p_eeprom_read_tlv_data);
	
    return sprintf(buf, "%s", cx532p_eeprom_read_tlv_data.block);
}
