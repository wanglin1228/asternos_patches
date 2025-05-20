/*
 *
 * Asterfusion lm_sensor device dirver
 * Version v1.0
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

ssize_t cx532p_caculate_temp(struct device *dev, struct device_attribute *attr, char *buf)
{
    u8 command = UART_CMD_TEMP;
    u8 data = 0, i = 0;
    struct sensor_device_attribute *da = to_sensor_dev_attr(attr);

    union i2c_smbus_data cx532p_lm_sensor_temp_data         = {.block = {0x00}};
    int ret = get_bmc_data(command, 0xaa, 0xaa, 3, &cx532p_lm_sensor_temp_data);
    if(da->index == LM_SENSOR_NUMBER) {
        for(i = 1; i < TEMP + 1; i++)
            if(cx532p_lm_sensor_temp_data.block[da->index] != 0 && cx532p_lm_sensor_temp_data.block[da->index] != 0xff)
                data++;
    }
    else
        data = cx532p_lm_sensor_temp_data.block[da->index];
    if (ret < 0)
        return sprintf(buf, "N/A\n");
    return sprintf(buf, "%d\n",data);
}
