/*
 *
 * Asterfusion psu device dirver
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

ssize_t cx532p_get_psu1_type(struct device *dev, struct device_attribute *attr, char *buf)
{
    u8 command = UART_CMD_PSU;
    u16 status;
    union i2c_smbus_data cx532p_psu_read_data = {.block={0x00}};
    get_bmc_data(command, 0x1, 0xaa, 3, &cx532p_psu_read_data);

    status = cx532p_psu_read_data.block[13];
    return sprintf(buf, "%d\n",status);
}


ssize_t cx532p_get_psu2_type(struct device *dev, struct device_attribute *attr, char *buf)
{
    u8 command = UART_CMD_PSU;
    u16 status;
    union i2c_smbus_data cx532p_psu_read_data = {.block={0x00}};
    get_bmc_data(command, 0x1, 0xaa, 3, &cx532p_psu_read_data);

    status = cx532p_psu_read_data.block[26];
    return sprintf(buf, "%d\n",status);
}

ssize_t cx532p_get_psu_status(struct device *dev, struct device_attribute *attr, char *buf)
{
    u8 command = UART_CMD_PSU;
    u8 data_1[BUFFERSIZE] = {0};
    u8 data_2[BUFFERSIZE] = {0};
    union i2c_smbus_data cx532p_psu_read_data = {.block={0x00}};
    int ret = get_bmc_data(command, 0x0, 0xaa, 3, &cx532p_psu_read_data);

    if (ret < 0)
    {
        sprintf(data_1, "PSU 1 is power unknown\n");
    }
    else if (!cx532p_psu_read_data.block[1])
    {
        sprintf(data_1, "PSU 1 is not power Good\n");
    }
    else
    {
        sprintf(data_1, "PSU 1 is power Good\n");
    }

    if (ret < 0)
    {
        sprintf(data_2, "PSU 2 is power unknown\n");
    }
    else if (!cx532p_psu_read_data.block[4])
    {
        sprintf(data_2, "PSU 2 is not power Good\n");
    }
    else
    {
        sprintf(data_2, "PSU 2 is power Good\n");
    }
    return sprintf(buf, "%s%s", data_1, data_2);
}

ssize_t cx532p_get_psu_present(struct device *dev, struct device_attribute *attr, char *buf)
{
    u8 command = UART_CMD_PSU;
    u8 data_1[BUFFERSIZE] = {0};
    u8 data_2[BUFFERSIZE] = {0};
    union i2c_smbus_data cx532p_psu_read_data = {.block={0x00}};
    int ret = get_bmc_data(command, 0x0, 0xaa, 3, &cx532p_psu_read_data);

    if (ret < 0)
    {
        sprintf(data_1, "PSU 1 is unknown\n");
    }
    else if (!cx532p_psu_read_data.block[3])
    {
        sprintf(data_1, "PSU 1 is present\n");
    }
    else
    {
        sprintf(data_1, "PSU 1 is not present\n");
    }

    if (ret < 0)
    {
        sprintf(data_2, "PSU 2 is unknown\n");
    }
    else if (!cx532p_psu_read_data.block[6])
    {
        sprintf(data_2, "PSU 2 is present\n");
    }
    else
    {
        sprintf(data_2, "PSU 2 is not present\n");
    }
    return sprintf(buf, "%s%s", data_1, data_2);
}

ssize_t cx532p_caculate_power(struct device *dev, struct device_attribute *attr, char *buf)
{
    u8 module_num = 0;
    u8 command = UART_CMD_PSU;
	int vin = 0, vout = 0, iin = 0, iout = 0, pout = 0, pin = 0;
    int i = 0;
    u8 data[BUFFERSIZE * 2] = {0};
	union i2c_smbus_data cx532p_psu_read_data = {.block={0x00}};

    struct sensor_device_attribute *da = to_sensor_dev_attr(attr);
    
    switch(da->index)
    {
        case PSU1_POWER:
            module_num = 1;
            break;
        case PSU2_POWER:
            module_num = 2;
            break;
    }
	
	get_bmc_data(command, 0x1, 0xaa, 3, &cx532p_psu_read_data);

	vin = (cx532p_psu_read_data.block[1 + (module_num-1)*13] & 0xff)*1000 + (cx532p_psu_read_data.block[2 + (module_num-1)*13] & 0xff)*100;
	vout = (cx532p_psu_read_data.block[3 + (module_num-1)*13] & 0xff)*1000 + (cx532p_psu_read_data.block[4 + (module_num-1)*13] & 0xff)*100;
	iin = (cx532p_psu_read_data.block[5 + (module_num-1)*13] & 0xff)*1000 + (cx532p_psu_read_data.block[6 + (module_num-1)*13] & 0xff)*100;
	iout = (cx532p_psu_read_data.block[7 + (module_num-1)*13] & 0xff)*1000 + (cx532p_psu_read_data.block[8 + (module_num-1)*13] & 0xff)*100;
	pout = (cx532p_psu_read_data.block[9 + (module_num-1)*13] & 0xff)*256 + (cx532p_psu_read_data.block[10 + (module_num-1)*13] & 0xff);
	pin = (cx532p_psu_read_data.block[11 + (module_num-1)*13] & 0xff)*256 + (cx532p_psu_read_data.block[12 + (module_num-1)*13] & 0xff);
    i = sprintf(data, "PSU %d VIN          is %d\n", module_num, vin);
    i += sprintf(data + i, "PSU %d VOUT         is %d\n", module_num, vout);
    i += sprintf(data + i, "PSU %d IIN          is %d\n", module_num, iin);
    i += sprintf(data + i, "PSU %d IOUT         is %d\n", module_num, iout);
    i += sprintf(data + i, "PSU %d POUT         is %d\n", module_num, pout);
    i += sprintf(data + i, "PSU %d PIN          is %d\n", module_num, pin);

    return sprintf(buf, "%s", data);
}

ssize_t cx532p_get_psu_direction(struct device *dev, struct device_attribute *attr, char *buf)
{
    u8 command = UART_CMD_PSU;
    u8 data_1[BUFFERSIZE] = {0};
    u8 data_2[BUFFERSIZE] = {0};
	union i2c_smbus_data cx532p_psu_read_data = {.block={0x00}};
    get_bmc_data(command, 0x0, 0xaa, 3, &cx532p_psu_read_data);

    if (!(cx532p_psu_read_data.block[3] & 0xf0))
    {
        sprintf(data_1, "PSU 1 is exhaust\n");
    }
    else
    {
        sprintf(data_1, "PSU 1 is intake\n");
    }
    if (!(cx532p_psu_read_data.block[6] & 0xf0))
    {
        sprintf(data_2, "PSU 2 is exhaust\n");
    }
    else
    {
        sprintf(data_2, "PSU 2 is intake\n");
    }
    return sprintf(buf, "%s%s", data_1, data_2);
}

ssize_t cx532p_get_psu_warning(struct device *dev, struct device_attribute *attr, char *buf)
{
    u8 command = UART_CMD_PSU;
    u8 data_1[BUFFERSIZE] = {0};
    u8 data_2[BUFFERSIZE] = {0};
	union i2c_smbus_data cx532p_psu_read_data = {.block={0x00}};
    get_bmc_data(command, 0x0, 0xaa, 3, &cx532p_psu_read_data);

    if (!(cx532p_psu_read_data.block[2] & 0xf))
    {
        sprintf(data_1, "PSU 1 is warning\n");
    }
    else
    {
        sprintf(data_1, "PSU 1 is not warning\n");
    }
    if (!(cx532p_psu_read_data.block[5] & 0xf))
    {
        sprintf(data_2, "PSU 2 is warning\n");
    }
    else
    {
        sprintf(data_2, "PSU 2 is not warning\n");
    }
    return sprintf(buf, "%s%s", data_1, data_2);
}

ssize_t cx532p_get_psu_direction_warning(struct device *dev, struct device_attribute *attr, char *buf)
{
    u8 command = UART_CMD_PSU;
    u8 data_1[BUFFERSIZE] = {0};
    u8 data_2[BUFFERSIZE] = {0};
	union i2c_smbus_data cx532p_psu_read_data = {.block={0x00}};
    get_bmc_data(command, 0x0, 0xaa, 3, &cx532p_psu_read_data);

    if (!(cx532p_psu_read_data.block[2] & 0xf0))
    {
        sprintf(data_1, "PSU 1 direction is not warning\n");
    }
    else
    {
        sprintf(data_1, "PSU 1 direction is warning\n");
    }
    if (!(cx532p_psu_read_data.block[5] & 0xf0))
    {
        sprintf(data_2, "PSU 2 direction is not warning\n");
    }
    else
    {
        sprintf(data_2, "PSU 2 direction is warning\n");
    }
    return sprintf(buf, "%s%s", data_1, data_2);
}

