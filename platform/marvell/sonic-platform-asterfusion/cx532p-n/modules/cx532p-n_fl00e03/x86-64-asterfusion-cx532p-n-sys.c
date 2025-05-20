/*
 *
 * Asterfusion core_voltage device dirver
 * Version v1.1
 *
 * Copyright (C) 2019 Tengfei <tengfei@asterfusion.com>
 * Copyright (C) 2019 Wangzhui <wangzhui@asterfusion.com>
 *
 *
 */
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

/* i2c_client Declaration */
extern struct i2c_client *aster_CPLD_40_client; //0x40 for Port 17-32
#define CPLD1_VERSION    0
#define CPLD2_VERSION    0x12
/* end of i2c_client Declaration */

ssize_t cx532p_read_core_voltage_high(struct device *dev, struct device_attribute *attr, char *buf)
{
    u8 command = UART_CMD_PAYLOAD_INFO;
    union i2c_smbus_data cx532p_core_voltage_read_data = {.block={0x00}};
    get_bmc_data(command, 0xaa, 0xaa, 3, &cx532p_core_voltage_read_data);
    return sprintf(buf, "0x%x\n", cx532p_core_voltage_read_data.block[2]);
}

ssize_t cx532p_read_core_voltage_low(struct device *dev, struct device_attribute *attr, char *buf)
{
    u8 command = UART_CMD_PAYLOAD_INFO;
    union i2c_smbus_data cx532p_core_voltage_read_data = {.block={0x00}};
    get_bmc_data(command, 0xaa, 0xaa, 3, &cx532p_core_voltage_read_data);
    return sprintf(buf, "0x%x\n", cx532p_core_voltage_read_data.block[1]);
}

ssize_t cx532p_shutdown_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u16 i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    union i2c_smbus_data cx532p_core_voltage_read_data = {.block={0x00}};
    
    if (attr->index == SHUTDOWN_DUT)
    {
        i = simple_strtol(buf, NULL, 10);
        if (i == 1)
        {
            get_bmc_data(UART_CMD_PAYLOAD_OP, 0xa1, 0xaa, 3, &cx532p_core_voltage_read_data);
        }
        else
        {
            printk(KERN_ALERT "shutdown_set set wrong Value\n");
        }
    }

    return count;
}

ssize_t bmc_version_get(struct device *dev, struct device_attribute *attr, char *buf)
{
    u8 command = UART_CMD_BMC_VERSION;
    union i2c_smbus_data bmc_version_read_data = {.block={0x00}};
    get_bmc_data(command, 0xaa, 0xaa, 3, &bmc_version_read_data);
    return sprintf(buf, "V%d.%dR%02d\n", bmc_version_read_data.block[1], bmc_version_read_data.block[2], bmc_version_read_data.block[3]);
}

ssize_t cpld_version_get(struct device *dev, struct device_attribute *attr, char *buf)
{
    u8 version_1   = -EPERM;
    u8 version_2   = -EPERM;
    version_1 = i2c_smbus_read_byte_data(aster_CPLD_40_client, CPLD1_VERSION);
    version_2 = i2c_smbus_read_byte_data(aster_CPLD_40_client, CPLD2_VERSION);
    return sprintf(buf, "CPLD1: %d\nCPLD2: %d\n", (version_1 & 0xff), (version_2 & 0xff));
}
