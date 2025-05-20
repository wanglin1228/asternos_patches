/*
 *
 * Asterfusion fan device dirver
 * Version v1.2
 *      Add support for both old_fan_board and new_fan_board
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
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/i2c-dev.h>
#include "x86-64-asterfusion-cx532p-n.h"
#include "x86-64-asterfusion-cx532p-n-common.h"


const u8 read_fan_code[] = {
0x00,0x01,0x02,0x03,0x04,
};

ssize_t cx532p_get_fan_board_type(struct device *dev, struct device_attribute *attr, char *buf)
{
    u8 command = UART_CMD_FAN_STATUS;
    union i2c_smbus_data cx532p_fan_state_data = {.block={0x00}};
    get_bmc_data(command, 0x00, 0x00, 3, &cx532p_fan_state_data);
    return sprintf(buf, "%x\n", cx532p_fan_state_data.block[3]);
}

ssize_t cx532p_fan_status_get(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u32 fan_index = attr->index;
    union i2c_smbus_data cx532p_fan_speed_data = {.block={0x00}};
    u8 data[BUFFERSIZE] = {0};
    int ret = get_bmc_data(UART_CMD_FAN_STATUS, fan_index, 0x1, 3, &cx532p_fan_speed_data);
    if(ret < 0)
    {
        sprintf(data, "Fan %d is unknown\n", fan_index);
    }
    else if(!(cx532p_fan_speed_data.block[1] & 0x2))
    {
        sprintf(data, "Fan %d is Good\n", fan_index);
    }
    else
    {
        sprintf(data, "Fan %d is Fail\n", fan_index);
    }
    return sprintf(buf, "%s", data);
}
ssize_t cx532p_fan_presence_get(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u32 fan_index = attr->index;
    union i2c_smbus_data cx532p_fan_speed_data = {.block={0x00}};
    u8 data[BUFFERSIZE] = {0};
    int ret = get_bmc_data(UART_CMD_FAN_STATUS, fan_index, 0x1, 3, &cx532p_fan_speed_data);
    if(ret < 0)
    {
        sprintf(data, "Fan %d is unknown\n", fan_index);
    }
    else if((cx532p_fan_speed_data.block[1] & 0x1))
    {
        sprintf(data, "Fan %d is present\n", fan_index);
    }
    else
    {
        sprintf(data, "Fan %d is not present\n", fan_index);
    } 
    return sprintf(buf, "%s\n",data);
}
ssize_t cx532p_fan_speed_rpm_get(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u32 fan_index = attr->index;
    u32 valid_data_length = 0;
    u16 front_speed_rpm = 0;
    u16 rear_speed_rpm = 0;
    union i2c_smbus_data cx532p_fan_speed_data = {.block={0x00}};
    u8 data[BUFFERSIZE] = {0};
    int ret = get_bmc_data(UART_CMD_FAN_STATUS, fan_index, 0x0, 3, &cx532p_fan_speed_data);
    valid_data_length = cx532p_fan_speed_data.block[0];
    switch (valid_data_length)
    {
        case 4:
            if (ret < 0)
            {
                sprintf(data, "FanModule%i Front : N/A\nFanModule%i Rear  : N/A\n", fan_index, fan_index);
                break;
            }
            front_speed_rpm = (cx532p_fan_speed_data.block[1] << 8) + cx532p_fan_speed_data.block[2];
            rear_speed_rpm = (cx532p_fan_speed_data.block[3] << 8) + cx532p_fan_speed_data.block[4];
            sprintf(data, "FanModule%i Front : %d\nFanModule%i Rear  : %d\n", fan_index, front_speed_rpm, fan_index, rear_speed_rpm);
            break;
        default:
            sprintf(data, "FanModule%i Front : N/A\nFanModule%i Rear  : N/A\n", fan_index, fan_index);
            break;
    }
    return sprintf(buf, "%s", data);
}

ssize_t cx532p_fan_direction_get(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u32 fan_index = attr->index;
    union i2c_smbus_data cx532p_fan_speed_data = {.block={0x00}};
    u8 data[BUFFERSIZE] = {0};
    int ret = get_bmc_data(UART_CMD_FAN_STATUS, fan_index, 0x1, 3, &cx532p_fan_speed_data);
    if(ret < 0)
    {
        sprintf(data, "Fan %d is unknown\n", fan_index);
    }
    else if((cx532p_fan_speed_data.block[2] & 0x1))
    {
        sprintf(data, "Fan %d is exhaust\n", fan_index);
    }
    else if((cx532p_fan_speed_data.block[2] & 0x2))
    {
        sprintf(data, "Fan %d is intake\n", fan_index);
    }
    else
    {
        sprintf(data, "Fan %d is normal\n", fan_index);
    } 
    return sprintf(buf, "%s\n",data);
}

