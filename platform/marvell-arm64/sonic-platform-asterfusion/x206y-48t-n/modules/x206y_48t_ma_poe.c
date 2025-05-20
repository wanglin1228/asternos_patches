/*
 * A POE driver for the X206Y-48T-ma
 *
 * Copyright (C) 2021 Asterfusion Corporation.
 * zhangliang <zhangliang@asterfusion.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/dmi.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>

#undef aster_DEBUG
/*#define aster_DEBUG*/
#ifdef aster_DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif /* DEBUG */

#define GET_BIT(data, bit, value)   value = (data >> bit) & 0x1
#define SET_BIT(data, bit)          data |= (1 << bit)
#define CLEAR_BIT(data, bit)        data &= ~(1 << bit)

#define PSE1_ADDRESS                0x22
#define PSE2_ADDRESS                0x23
#define PSE3_ADDRESS                0x24
#define PSE4_ADDRESS                0x25

#define PSE5_ADDRESS                0x26
#define PSE6_ADDRESS                0x27
#define PSE7_ADDRESS                0x28
#define PSE8_ADDRESS                0x29

#define PSE9_ADDRESS                0x2c
#define PSE10_ADDRESS               0x2d
#define PSE11_ADDRESS               0x30
#define PSE12_ADDRESS               0x31

#define PSE_ALL_ADDRESS             0x7f

#define POE_CONTROL_REG             0x12
#define POE_CONTROL_2_REG           0x14

#define PSE_TEMP_REG                0x2c
#define PSE_INPUT_VOL_REG           0x2e

#define PSE_PORT1_CUR_REG           0x30
#define PSE_PORT2_CUR_REG           0x34
#define PSE_PORT3_CUR_REG           0x38
#define PSE_PORT4_CUR_REG           0x3c
#define PSE_PORT1_VOL_REG           0x32
#define PSE_PORT2_VOL_REG           0x36
#define PSE_PORT3_VOL_REG           0x3a
#define PSE_PORT4_VOL_REG           0x3e

static LIST_HEAD(pse_client_list);
static struct mutex  list_lock;

/* Addresses scanned for asterfusion_x206y48t_ma_cpld */
static const unsigned short normal_i2c[] = { PSE1_ADDRESS, PSE2_ADDRESS, PSE3_ADDRESS, PSE4_ADDRESS, PSE5_ADDRESS, \
                                        PSE6_ADDRESS, PSE7_ADDRESS, PSE8_ADDRESS, PSE9_ADDRESS, PSE10_ADDRESS, \
                                        PSE11_ADDRESS, PSE12_ADDRESS, I2C_CLIENT_END };

struct pse_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

int asterfusion_x206y48t_ma_pse_read(unsigned short addr, u8 reg)
{
    struct list_head   *list_node = NULL;
    struct pse_client_node *pse_node = NULL;
    int data = -EPERM;
    
    mutex_lock(&list_lock);

    list_for_each(list_node, &pse_client_list)
    {
        pse_node = list_entry(list_node, struct pse_client_node, list);
        
        if (pse_node->client->addr == addr) {
            data = i2c_smbus_read_byte_data(pse_node->client, reg);
            DBG(printk(KERN_ALERT "%s:%d - addr: 0x%x, reg: %x, data: %x\r\n", __func__, __LINE__, addr, reg, data));
            break;
        }
    }
    
    mutex_unlock(&list_lock);

    return data;
}
EXPORT_SYMBOL(asterfusion_x206y48t_ma_pse_read);


int asterfusion_x206y48t_ma_pse_read_word(unsigned short addr, u8 reg)
{
    struct list_head   *list_node = NULL;
    struct pse_client_node *pse_node = NULL;
    int data = -EPERM;
    
    mutex_lock(&list_lock);

    list_for_each(list_node, &pse_client_list)
    {
        pse_node = list_entry(list_node, struct pse_client_node, list);
        
        if (pse_node->client->addr == addr) {
            data = i2c_smbus_read_word_data(pse_node->client, reg);
            DBG(printk(KERN_ALERT "%s:%d - addr: 0x%x, reg: %x, data: %x\r\n", __func__, __LINE__, addr, reg, data));
            break;
        }
    }
    
    mutex_unlock(&list_lock);

    return data;
}
EXPORT_SYMBOL(asterfusion_x206y48t_ma_pse_read_word);

int asterfusion_x206y48t_ma_pse_write(unsigned short addr, u8 reg, u8 val)
{
    struct list_head   *list_node = NULL;
    struct pse_client_node *pse_node = NULL;
    int ret = -EIO;
    
    mutex_lock(&list_lock);

    list_for_each(list_node, &pse_client_list)
    {
        pse_node = list_entry(list_node, struct pse_client_node, list);
        
        if (pse_node->client->addr == addr) {
            ret = i2c_smbus_write_byte_data(pse_node->client, reg, val);
             DBG(printk(KERN_ALERT "%s:%d - addr: 0x%x, reg: %x, data: %x\r\n", __func__, __LINE__, addr, reg, val));
            break;
        }
    }
    
    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(asterfusion_x206y48t_ma_pse_write);

static ssize_t show_port1_poe_1(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = POE_CONTROL_REG;

    data = asterfusion_x206y48t_ma_pse_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data &= 0x3;

    return sprintf(buf, "%02x\n", data);
}

static ssize_t set_port1_poe_1(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = POE_CONTROL_REG;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    data = asterfusion_x206y48t_ma_pse_read(client->addr, reg);
    data = val | (data & 0xFC);

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    asterfusion_x206y48t_ma_pse_write(client->addr, reg, data);

    return count;
}

static ssize_t show_port2_poe_1(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = POE_CONTROL_REG;

    data = asterfusion_x206y48t_ma_pse_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data = (data >> 2) & 0x3;
 
    return sprintf(buf, "%02x\n", data);
}

static ssize_t set_port2_poe_1(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = POE_CONTROL_REG;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    data = asterfusion_x206y48t_ma_pse_read(client->addr, reg);
    data = (val << 2) | (data & 0xF3);

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    asterfusion_x206y48t_ma_pse_write(client->addr, reg, data);

    return count;
}

static ssize_t show_port3_poe_1(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = POE_CONTROL_REG;

    data = asterfusion_x206y48t_ma_pse_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data = (data >> 4) & 0x3;

    return sprintf(buf, "%02x\n", data);
}

static ssize_t set_port3_poe_1(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = POE_CONTROL_REG;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    data = asterfusion_x206y48t_ma_pse_read(client->addr, reg);
    data = (val << 4) | (data & 0xCF);

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    asterfusion_x206y48t_ma_pse_write(client->addr, reg, data);

    return count;
}

static ssize_t show_port4_poe_1(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = POE_CONTROL_REG;

    data = asterfusion_x206y48t_ma_pse_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data = (data >> 6) & 0x3;

    return sprintf(buf, "%02x\n", data);
}

static ssize_t set_port4_poe_1(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = POE_CONTROL_REG;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    data = asterfusion_x206y48t_ma_pse_read(client->addr, reg);
    data = (val << 6) | (data & 0x3F);

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    asterfusion_x206y48t_ma_pse_write(client->addr, reg, data);

    return count;
}

/* maurice note PSE Register for port detect or class enable */
static ssize_t show_port1_poe_2(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = POE_CONTROL_2_REG;

    data = asterfusion_x206y48t_ma_pse_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data &= 0x11;

    return sprintf(buf, "%02x\n", data);
}

static ssize_t set_port1_poe_2(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = POE_CONTROL_2_REG;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    data = asterfusion_x206y48t_ma_pse_read(client->addr, reg);
    data = (val & 0x11) | (data & 0xEE);

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    asterfusion_x206y48t_ma_pse_write(client->addr, reg, data);

    return count;
}

static ssize_t show_port2_poe_2(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = POE_CONTROL_2_REG;

    data = asterfusion_x206y48t_ma_pse_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data &= 0x22;

    return sprintf(buf, "%02x\n", data);
}

static ssize_t set_port2_poe_2(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = POE_CONTROL_2_REG;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    data = asterfusion_x206y48t_ma_pse_read(client->addr, reg);
    data = (val & 0x22) | (data & 0xDD);

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    asterfusion_x206y48t_ma_pse_write(client->addr, reg, data);

    return count;
}

static ssize_t show_port3_poe_2(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = POE_CONTROL_2_REG;

    data = asterfusion_x206y48t_ma_pse_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data &= 0x44;

    return sprintf(buf, "%02x\n", data);
}

static ssize_t set_port3_poe_2(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = POE_CONTROL_2_REG;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    data = asterfusion_x206y48t_ma_pse_read(client->addr, reg);
    data = (val & 0x44) | (data & 0xBB);

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    asterfusion_x206y48t_ma_pse_write(client->addr, reg, data);

    return count;
}

static ssize_t show_port4_poe_2(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = POE_CONTROL_2_REG;

    data = asterfusion_x206y48t_ma_pse_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data &= 0x88;

    return sprintf(buf, "%02x\n", data);
}

static ssize_t set_port4_poe_2(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = POE_CONTROL_2_REG;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    data = asterfusion_x206y48t_ma_pse_read(client->addr, reg);
    data = (val & 0x88) | (data & 0x77);

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    asterfusion_x206y48t_ma_pse_write(client->addr, reg, data);

    return count;
}

static ssize_t show_temperature(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u16 data = 0;
    u8 reg = PSE_TEMP_REG;

    data = asterfusion_x206y48t_ma_pse_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data &= 0xff;

    return sprintf(buf, "%02x\n", data);
}

static ssize_t show_input_voltage(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u16 data = 0;
    u8 reg = PSE_INPUT_VOL_REG;

    data = asterfusion_x206y48t_ma_pse_read_word(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data &= 0x3fff;

    return sprintf(buf, "%04x\n", data);
}

static ssize_t show_port1_current(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u16 data = 0;
    u8 reg = PSE_PORT1_CUR_REG;

    data = asterfusion_x206y48t_ma_pse_read_word(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data &= 0x3fff;

    return sprintf(buf, "%04x\n", data);
}

static ssize_t show_port2_current(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u16 data = 0;
    u8 reg = PSE_PORT2_CUR_REG;

    data = asterfusion_x206y48t_ma_pse_read_word(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data &= 0x3fff;

    return sprintf(buf, "%04x\n", data);
}

static ssize_t show_port3_current(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u16 data = 0;
    u8 reg = PSE_PORT3_CUR_REG;

    data = asterfusion_x206y48t_ma_pse_read_word(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data &= 0x3fff;

    return sprintf(buf, "%04x\n", data);
}

static ssize_t show_port4_current(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u16 data = 0;
    u8 reg = PSE_PORT4_CUR_REG;

    data = asterfusion_x206y48t_ma_pse_read_word(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data &= 0x3fff;

    return sprintf(buf, "%04x\n", data);
}

static ssize_t show_port1_voltage(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u16 data = 0;
    u8 reg = PSE_PORT1_VOL_REG;

    data = asterfusion_x206y48t_ma_pse_read_word(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data &= 0x3fff;


    return sprintf(buf, "%04x\n", data);
}

static ssize_t show_port2_voltage(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u16 data = 0;
    u8 reg = PSE_PORT2_VOL_REG;

    data = asterfusion_x206y48t_ma_pse_read_word(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data &= 0x3fff;

    return sprintf(buf, "%04x\n", data);
}

static ssize_t show_port3_voltage(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u16 data = 0;
    u8 reg = PSE_PORT3_VOL_REG;

    data = asterfusion_x206y48t_ma_pse_read_word(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data &= 0x3fff;

    return sprintf(buf, "%04x\n", data);
}

static ssize_t show_port4_voltage(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u16 data = 0;
    u8 reg = PSE_PORT4_VOL_REG;

    data = asterfusion_x206y48t_ma_pse_read_word(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data &= 0x3fff;

    return sprintf(buf, "%04x\n", data);
}

static SENSOR_DEVICE_ATTR(pse_port1_mode,  S_IRUGO | S_IWUSR, show_port1_poe_1, set_port1_poe_1, 0);
static SENSOR_DEVICE_ATTR(pse_port2_mode,  S_IRUGO | S_IWUSR, show_port2_poe_1, set_port2_poe_1, 0);
static SENSOR_DEVICE_ATTR(pse_port3_mode,  S_IRUGO | S_IWUSR, show_port3_poe_1, set_port3_poe_1, 0);
static SENSOR_DEVICE_ATTR(pse_port4_mode,  S_IRUGO | S_IWUSR, show_port4_poe_1, set_port4_poe_1, 0);

static SENSOR_DEVICE_ATTR(pse_port1_dc_enable,  S_IRUGO | S_IWUSR, show_port1_poe_2, set_port1_poe_2, 0);
static SENSOR_DEVICE_ATTR(pse_port2_dc_enable,  S_IRUGO | S_IWUSR, show_port2_poe_2, set_port2_poe_2, 0);
static SENSOR_DEVICE_ATTR(pse_port3_dc_enable,  S_IRUGO | S_IWUSR, show_port3_poe_2, set_port3_poe_2, 0);
static SENSOR_DEVICE_ATTR(pse_port4_dc_enable,  S_IRUGO | S_IWUSR, show_port4_poe_2, set_port4_poe_2, 0);

static SENSOR_DEVICE_ATTR(pse_temperature,  S_IRUGO, show_temperature, NULL, 0);
static SENSOR_DEVICE_ATTR(pse_input_voltage,  S_IRUGO, show_input_voltage, NULL, 0);

static SENSOR_DEVICE_ATTR(pse_port1_current,  S_IRUGO, show_port1_current, NULL, 0);
static SENSOR_DEVICE_ATTR(pse_port2_current,  S_IRUGO, show_port2_current, NULL, 0);
static SENSOR_DEVICE_ATTR(pse_port3_current,  S_IRUGO, show_port3_current, NULL, 0);
static SENSOR_DEVICE_ATTR(pse_port4_current,  S_IRUGO, show_port4_current, NULL, 0);

static SENSOR_DEVICE_ATTR(pse_port1_voltage,  S_IRUGO, show_port1_voltage, NULL, 0);
static SENSOR_DEVICE_ATTR(pse_port2_voltage,  S_IRUGO, show_port2_voltage, NULL, 0);
static SENSOR_DEVICE_ATTR(pse_port3_voltage,  S_IRUGO, show_port3_voltage, NULL, 0);
static SENSOR_DEVICE_ATTR(pse_port4_voltage,  S_IRUGO, show_port4_voltage, NULL, 0);

static struct attribute *asterfusion_x206y48t_ma_pse_attributes[] = {
    &sensor_dev_attr_pse_port1_mode.dev_attr.attr,
    &sensor_dev_attr_pse_port2_mode.dev_attr.attr,
    &sensor_dev_attr_pse_port3_mode.dev_attr.attr,
    &sensor_dev_attr_pse_port4_mode.dev_attr.attr,
    &sensor_dev_attr_pse_port1_dc_enable.dev_attr.attr,
    &sensor_dev_attr_pse_port2_dc_enable.dev_attr.attr,
    &sensor_dev_attr_pse_port3_dc_enable.dev_attr.attr,
    &sensor_dev_attr_pse_port4_dc_enable.dev_attr.attr,

    &sensor_dev_attr_pse_temperature.dev_attr.attr,
    &sensor_dev_attr_pse_input_voltage.dev_attr.attr,
    
    &sensor_dev_attr_pse_port1_current.dev_attr.attr,
    &sensor_dev_attr_pse_port2_current.dev_attr.attr,
    &sensor_dev_attr_pse_port3_current.dev_attr.attr,
    &sensor_dev_attr_pse_port4_current.dev_attr.attr,
    &sensor_dev_attr_pse_port1_voltage.dev_attr.attr,
    &sensor_dev_attr_pse_port2_voltage.dev_attr.attr,
    &sensor_dev_attr_pse_port3_voltage.dev_attr.attr,
    &sensor_dev_attr_pse_port4_voltage.dev_attr.attr,
    NULL
};

static const struct attribute_group asterfusion_x206y48t_ma_pse_group = { .attrs = asterfusion_x206y48t_ma_pse_attributes};

static ssize_t show_port_all_poe_1(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = POE_CONTROL_REG;

    data = asterfusion_x206y48t_ma_pse_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data &= 0xFF;

    return sprintf(buf, "%02x\n", data);
}

static ssize_t set_port_all_poe_1(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = POE_CONTROL_REG;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    data = val & 0xFF;
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    asterfusion_x206y48t_ma_pse_write(client->addr, reg, data);

    return count;
}

static ssize_t show_port_all_poe_2(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = POE_CONTROL_2_REG;

    data = asterfusion_x206y48t_ma_pse_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data &= 0xFF;

    return sprintf(buf, "%02x\n", data);
}

static ssize_t set_port_all_poe_2(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = POE_CONTROL_2_REG;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    data = val & 0xFF;
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    asterfusion_x206y48t_ma_pse_write(client->addr, reg, data);

    return count;
}

static SENSOR_DEVICE_ATTR(pse_port_all_mode,  S_IRUGO | S_IWUSR, show_port_all_poe_1, set_port_all_poe_1, 0);
static SENSOR_DEVICE_ATTR(pse_port_all_dc_enable,  S_IRUGO | S_IWUSR, show_port_all_poe_2, set_port_all_poe_2, 0);

static struct attribute *asterfusion_x206y48t_ma_pse_all_attributes[] = {
    &sensor_dev_attr_pse_port_all_mode.dev_attr.attr,
    &sensor_dev_attr_pse_port_all_dc_enable.dev_attr.attr,
    NULL
};

static const struct attribute_group asterfusion_x206y48t_ma_pse_all_group = { .attrs = asterfusion_x206y48t_ma_pse_all_attributes};

static void asterfusion_x206y48t_ma_pse_add_client(struct i2c_client *client)
{
    struct pse_client_node *node = kzalloc(sizeof(struct pse_client_node), GFP_KERNEL);
    
    if (!node) {
        dev_dbg(&client->dev, "Can't allocate pse_client_node (0x%x)\n", client->addr);
        return;
    }
    
    node->client = client;
    
    mutex_lock(&list_lock);
    list_add(&node->list, &pse_client_list);
    mutex_unlock(&list_lock);
}

static void asterfusion_x206y48t_ma_pse_remove_client(struct i2c_client *client)
{
    struct list_head        *list_node = NULL;
    struct pse_client_node *pse_node = NULL;
    int found = 0;

    mutex_lock(&list_lock);

    list_for_each(list_node, &pse_client_list)
    {
        pse_node = list_entry(list_node, struct pse_client_node, list);
        
        if (pse_node->client == client) {
            found = 1;
            break;
        }
    }

    if (found) {
        list_del(list_node);
        kfree(pse_node);
    }
    mutex_unlock(&list_lock);
}

static int asterfusion_x206y48t_ma_pse_probe(struct i2c_client *client,
            const struct i2c_device_id *dev_id)
{
    int status;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_dbg(&client->dev, "i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    /* Register sysfs hooks */
    switch(client->addr)
    {
        case PSE1_ADDRESS:
        case PSE2_ADDRESS:
        case PSE3_ADDRESS:
        case PSE4_ADDRESS:
        case PSE5_ADDRESS:
        case PSE6_ADDRESS:
        case PSE7_ADDRESS:
        case PSE8_ADDRESS:
        case PSE9_ADDRESS:
        case PSE10_ADDRESS:
        case PSE11_ADDRESS:
        case PSE12_ADDRESS:
            status = sysfs_create_group(&client->dev.kobj, &asterfusion_x206y48t_ma_pse_group);
            break;
        case PSE_ALL_ADDRESS:
            status = sysfs_create_group(&client->dev.kobj, &asterfusion_x206y48t_ma_pse_all_group);
            break;
        default:
            dev_dbg(&client->dev, "i2c_check_pse failed (0x%x)\n", client->addr);
            status = -EIO;
            goto exit;
            break;
    }

    if (status) {
        goto exit;
    }

    dev_info(&client->dev, "chip found\n");
    asterfusion_x206y48t_ma_pse_add_client(client);
    
    return 0; 

exit:
    return status;
}

static int asterfusion_x206y48t_ma_pse_remove(struct i2c_client *client)
{
    switch(client->addr)
    {
        case PSE1_ADDRESS:
        case PSE2_ADDRESS:
        case PSE3_ADDRESS:
        case PSE4_ADDRESS:
        case PSE5_ADDRESS:
        case PSE6_ADDRESS:
        case PSE7_ADDRESS:
        case PSE8_ADDRESS:
        case PSE9_ADDRESS:
        case PSE10_ADDRESS:
        case PSE11_ADDRESS:
        case PSE12_ADDRESS:
            sysfs_remove_group(&client->dev.kobj, &asterfusion_x206y48t_ma_pse_group);
            break;
        case PSE_ALL_ADDRESS:
            sysfs_remove_group(&client->dev.kobj, &asterfusion_x206y48t_ma_pse_all_group);
            break;
        default:
            dev_dbg(&client->dev, "i2c_remove_pse failed (0x%x)\n", client->addr);
            break;
    }

    asterfusion_x206y48t_ma_pse_remove_client(client);  
    return 0;
}

static const struct i2c_device_id asterfusion_x206y48t_ma_pse_id[] = {
    { "x206y48t_ma_pse", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, asterfusion_x206y48t_ma_pse_id);

static struct i2c_driver asterfusion_x206y48t_ma_pse_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name = "asterfusion_x206y48t_ma_pse",
    },
    .probe      = asterfusion_x206y48t_ma_pse_probe,
    .remove     = asterfusion_x206y48t_ma_pse_remove,
    .id_table   = asterfusion_x206y48t_ma_pse_id,
    .address_list = normal_i2c,
};

static int __init asterfusion_x206y48t_ma_pse_init(void)
{
    mutex_init(&list_lock);

    return i2c_add_driver(&asterfusion_x206y48t_ma_pse_driver);
}

static void __exit asterfusion_x206y48t_ma_pse_exit(void)
{
    i2c_del_driver(&asterfusion_x206y48t_ma_pse_driver);
}

MODULE_AUTHOR("zhangliang <zhangliang@asterfusion.com>");
MODULE_DESCRIPTION("asterfusion_x206y48t_ma_pse driver");
MODULE_LICENSE("GPL");

module_init(asterfusion_x206y48t_ma_pse_init);
module_exit(asterfusion_x206y48t_ma_pse_exit);
