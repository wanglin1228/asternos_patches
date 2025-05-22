/*
 * A CPLD driver for the cx206y_48t
 *
 * Copyright (C) 2018 asterfusion Corporation.
 * Wang_Lin <wanglin@asterfusion.com>
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

#define CPLD_SFP_MAX_GROUP          1
//#define QSFP_FIRST_PORT             48
#define CPLD_ADDRESS                0x75

#define CPLD_VERSION_REG            0x0
#define LED_CONTROL1_REG            0x5
#define LED_CONTROL2_REG            0x6
//#define CPLD_EEPROM_WRITE_REG       0x12
#define CPLD_PSU_REG                0x3

#define SFP_STATUS_BASE             0x9
#define SFP_STATUS_TXDISABLE_BASE   0xc

#if 0
#define QSFP_PRESENT_ADDRESS        0xF
#define QSFP_RESET_ADDRESS_BASE     0x10
#define QSFP_MODSELN_ADDRESS        0x17
#define QSFP_LOW_POWER_ADDRESS      0x18
#endif

#define CPLD_SERIAL_LED_BIT         3 /* maurice for SLED_EN */
//#define CPLD_EEPROM_WRITE_BIT       2
#define SFP_PRESENT_BASE            0
#define SFP_RXLOSS_BASE             1
#define SFP_TXFAULT_BASE            2

#define CPLD_PSU_ACOK_BASE          0
#define CPLD_PSU_PWOK_BASE          2
#define CPLD_PSU_PRESENT_BASE       4
#define GET_BIT(data, bit, value)   value = (data >> bit) & 0x1
#define SET_BIT(data, bit)          data |= (1 << bit)
#define CLEAR_BIT(data, bit)        data &= ~(1 << bit)

static LIST_HEAD(cpld_client_list);
static struct mutex  list_lock;
/* Addresses scanned for cx206y_48t_cpld
 */
static const unsigned short normal_i2c[] = { CPLD_ADDRESS, I2C_CLIENT_END };

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

int cx206y_48t_cpld_read(unsigned short addr, u8 reg)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int data = -EPERM;
    
    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);
        
        if (cpld_node->client->addr == addr) {
            data = i2c_smbus_read_byte_data(cpld_node->client, reg);
            DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, addr, reg, data));
            break;
        }
    }
    
    mutex_unlock(&list_lock);

    return data;
}
EXPORT_SYMBOL(cx206y_48t_cpld_read);


int cx206y_48t_cpld_write(unsigned short addr, u8 reg, u8 val)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EIO;
    
    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);
        
        if (cpld_node->client->addr == addr) {
            ret = i2c_smbus_write_byte_data(cpld_node->client, reg, val);
             DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, addr, reg, val));
            break;
        }
    }
    
    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(cx206y_48t_cpld_write);

static ssize_t read_cpld_HWversion(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = CPLD_VERSION_REG;

    data = cx206y_48t_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));

    return sprintf(buf, "%02x", (data >> 5) & 0x7);
}

static ssize_t read_cpld_SWversion(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = CPLD_VERSION_REG;

    data = cx206y_48t_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));

    return sprintf(buf, "%02x", (data & 0x1f));
}

/* CPLD Register for Port LED */
static ssize_t show_allled_ctrl(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = LED_CONTROL1_REG;

    data = cx206y_48t_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data &= 0x3;

    return sprintf(buf, "%02x\n", data);
}

static ssize_t set_allled_ctrl(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = LED_CONTROL1_REG;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    data = cx206y_48t_cpld_read(client->addr, reg);
    data = val | (data & 0xfc);

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    cx206y_48t_cpld_write(client->addr, reg, data);

    return count;
}

static ssize_t show_serial_led(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0, reg = LED_CONTROL2_REG;

    data = cx206y_48t_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, CPLD_SERIAL_LED_BIT, val);

    return sprintf(buf, "%02x\n", val);
}

static ssize_t set_serial_led(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = LED_CONTROL2_REG;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }
    
    data = cx206y_48t_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    if(val)
        SET_BIT(data, CPLD_SERIAL_LED_BIT);
    else
        CLEAR_BIT(data, CPLD_SERIAL_LED_BIT);
    
    cx206y_48t_cpld_write(client->addr, reg, data);

    return count;
}

static ssize_t show_sys_led(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = LED_CONTROL1_REG;

    data = cx206y_48t_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data = (data >> 5) & 0x7;

    return sprintf(buf, "%02x\n", data);
}

static ssize_t set_sys_led(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = LED_CONTROL1_REG;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    data = cx206y_48t_cpld_read(client->addr, reg);
    data = (val << 5) | (data & 0x1f);

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    cx206y_48t_cpld_write(client->addr, reg, data);

    return count;
}
static ssize_t show_pwr_led(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = LED_CONTROL1_REG;

    data = cx206y_48t_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data = (data >> 2) & 0x7;

    return sprintf(buf, "%02x\n", data);
}

static ssize_t set_pwr_led(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = LED_CONTROL1_REG;
    long val = 0;


    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    data = cx206y_48t_cpld_read(client->addr, reg);
    data = (val << 2) | (data & 0xe3);

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    cx206y_48t_cpld_write(client->addr, reg, data);

    return count;
}
static ssize_t show_loc_led(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = LED_CONTROL2_REG;

    data = cx206y_48t_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data = (data>>4) & 0x3;

    return sprintf(buf, "%02x\n", data);
}

static ssize_t set_loc_led(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = LED_CONTROL2_REG;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    data = cx206y_48t_cpld_read(client->addr, reg);
    data = (val << 4) | (data & 0xf);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    cx206y_48t_cpld_write(client->addr, reg, data);

    return count;
}

static ssize_t show_fan_led(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = LED_CONTROL2_REG;

    data = cx206y_48t_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data &= 0x7;

    return sprintf(buf, "%02x\n", data);
}

static ssize_t set_fan_led(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = LED_CONTROL2_REG;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    data = cx206y_48t_cpld_read(client->addr, reg);
    data = val | (data & 0xf8);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    cx206y_48t_cpld_write(client->addr, reg, data);

    return count;
}

#if 0
static ssize_t show_eeprom_write_enable(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0, reg = CPLD_EEPROM_WRITE_REG;

    data = cx206y_48t_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, reg, val);

    return sprintf(buf, "%02x\n", val);
}

static ssize_t set_eeprom_write_enable(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = CPLD_EEPROM_WRITE_REG;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }
    
    data = cx206y_48t_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    if(val)
        SET_BIT(data, CPLD_EEPROM_WRITE_BIT);
    else
        CLEAR_BIT(data, CPLD_EEPROM_WRITE_BIT);
    
    cx206y_48t_cpld_write(client->addr, reg, data);

    return count;
}
#endif

static ssize_t read_psu_present(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0, reg = CPLD_PSU_REG;

    data = cx206y_48t_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, (CPLD_PSU_PRESENT_BASE + attr->index), val);

    return sprintf(buf, "%02x\n", val);
}

static ssize_t read_psu_pw_status(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val=0, reg = CPLD_PSU_REG;

    data = cx206y_48t_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, (CPLD_PSU_PWOK_BASE + attr->index), val);

    return sprintf(buf, "%02x\n", val);
}

static ssize_t read_psu_ac_status(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val=0, reg = CPLD_PSU_REG;

    data = cx206y_48t_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, (CPLD_PSU_ACOK_BASE + attr->index), val);

    return sprintf(buf, "%02x\n", val);
}

#define GET_SFP_STATUS_ADDRESS(idx, reg) \
        reg = SFP_STATUS_BASE + (idx / 2); \

static ssize_t get_sfp_present(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 reg = 0, data = 0, val = 0;

    GET_SFP_STATUS_ADDRESS(attr->index, reg);
    data = cx206y_48t_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, (SFP_PRESENT_BASE + 4*(attr->index % 2)), val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_sfp_tx_disable(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 reg = 0, data = 0, val = 0;

    reg = SFP_STATUS_TXDISABLE_BASE;

    data = cx206y_48t_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, (attr->index - 1), val);

    return sprintf(buf, "%d\n", val);
}
static ssize_t set_sfp_tx_disable(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 reg = 0, data = 0;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    reg = SFP_STATUS_TXDISABLE_BASE;
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data = cx206y_48t_cpld_read(client->addr, reg);

    if(val)
        SET_BIT(data, (attr->index - 1));
    else
        CLEAR_BIT(data, (attr->index - 1));

    cx206y_48t_cpld_write(client->addr, reg, data);

    return count;
}
static ssize_t get_sfp_rx_loss(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 reg = 0, data = 0, val = 0;

    GET_SFP_STATUS_ADDRESS(attr->index, reg);
    
    data = cx206y_48t_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, (SFP_RXLOSS_BASE + 4*(attr->index % 2)), val);

    return sprintf(buf, "%d\n", val);
}
static ssize_t get_sfp_tx_fault(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 reg = 0, data = 0, val = 0;

    GET_SFP_STATUS_ADDRESS(attr->index, reg);
    
    data = cx206y_48t_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, (SFP_TXFAULT_BASE + 4*(attr->index % 2)), val);

    return sprintf(buf, "%d\n",val);
}
#if 0
static ssize_t get_qsfp_present(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0, reg = QSFP_PRESENT_ADDRESS;

    data = cx206y_48t_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, (attr->index % QSFP_FIRST_PORT), val);

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_qsfp_reset(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 reg = (QSFP_RESET_ADDRESS_BASE + attr->index % QSFP_FIRST_PORT / 4), data =0;

    data = cx206y_48t_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    data = (data >> ((attr->index % QSFP_FIRST_PORT % 4)*2)) & 0x3;

    return sprintf(buf, "%d\n", data);
}

static ssize_t set_qsfp_reset(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 reg = (QSFP_RESET_ADDRESS_BASE + attr->index % QSFP_FIRST_PORT / 4), data = 0;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }

    data = cx206y_48t_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    CLEAR_BIT(data, (attr->index % 4)*2);
    CLEAR_BIT(data, (attr->index % 4)*2+1);
    data |= (val & 0x3) << ((attr->index % QSFP_FIRST_PORT % 4)*2);

    cx206y_48t_cpld_write(client->addr, reg, data);

    return count;
}

static ssize_t get_qsfp_lowpower(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0, reg = QSFP_LOW_POWER_ADDRESS;

    data = cx206y_48t_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, (attr->index % QSFP_FIRST_PORT), val);
    return sprintf(buf, "%02x\n", val);
}

static ssize_t set_qsfp_lowpower(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = QSFP_LOW_POWER_ADDRESS;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }
    data = cx206y_48t_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    if(val)
        SET_BIT(data, (attr->index % QSFP_FIRST_PORT));
    else
        CLEAR_BIT(data, (attr->index % QSFP_FIRST_PORT));

    cx206y_48t_cpld_write(client->addr, reg, data);

    return count;
}

static ssize_t get_qsfp_modeseln(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, val = 0, reg = QSFP_MODSELN_ADDRESS;

    data = cx206y_48t_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    GET_BIT(data, (attr->index % QSFP_FIRST_PORT), val);
    return sprintf(buf, "%02x\n", val);
}

static ssize_t set_qsfp_modeseln(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = QSFP_MODSELN_ADDRESS;
    long val = 0;

    if (kstrtol(buf, 16, &val))
    {
        return -EINVAL;
    }
    data = cx206y_48t_cpld_read(client->addr, reg);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, data));
    if(val)
        SET_BIT(data, (attr->index % QSFP_FIRST_PORT));
    else
        CLEAR_BIT(data, (attr->index % QSFP_FIRST_PORT));

    cx206y_48t_cpld_write(client->addr, reg, data);

    return count;
}
#endif
static SENSOR_DEVICE_ATTR(cpld_hw_version,  S_IRUGO, read_cpld_HWversion, NULL, 0);
static SENSOR_DEVICE_ATTR(cpld_sw_version,  S_IRUGO, read_cpld_SWversion, NULL, 0);

/* CPLD Register for Port LED */
static SENSOR_DEVICE_ATTR(cpld_allled_ctrl,  S_IRUGO | S_IWUSR, show_allled_ctrl, set_allled_ctrl, 0);
static SENSOR_DEVICE_ATTR(serial_led_enable,  S_IRUGO | S_IWUSR, show_serial_led, set_serial_led, 0);

static SENSOR_DEVICE_ATTR(sys_led,  S_IRUGO | S_IWUSR, show_sys_led, set_sys_led, 0);
static SENSOR_DEVICE_ATTR(pwr_led,  S_IRUGO | S_IWUSR, show_pwr_led, set_pwr_led, 0);
static SENSOR_DEVICE_ATTR(loc_led,  S_IRUGO | S_IWUSR, show_loc_led, set_loc_led, 0);
static SENSOR_DEVICE_ATTR(fan_led,  S_IRUGO | S_IWUSR, show_fan_led, set_fan_led, 0);
//static SENSOR_DEVICE_ATTR(eeprom_write_enable,  S_IRUGO | S_IWUSR, show_eeprom_write_enable, set_eeprom_write_enable, 0);
static SENSOR_DEVICE_ATTR(psu_1_present,  S_IRUGO, read_psu_present, NULL, 0);
static SENSOR_DEVICE_ATTR(psu_2_present,  S_IRUGO, read_psu_present, NULL, 1);
static SENSOR_DEVICE_ATTR(psu_1_pw_status,  S_IRUGO, read_psu_pw_status, NULL, 0);
static SENSOR_DEVICE_ATTR(psu_2_pw_status,  S_IRUGO, read_psu_pw_status, NULL, 1);
static SENSOR_DEVICE_ATTR(psu_1_ac_status,  S_IRUGO, read_psu_ac_status, NULL, 0);
static SENSOR_DEVICE_ATTR(psu_2_ac_status,  S_IRUGO, read_psu_ac_status, NULL, 1);

#define SET_SFP_ATTR(_num)  \
        static SENSOR_DEVICE_ATTR(sfp##_num##_present,  S_IRUGO, get_sfp_present, NULL, _num-49);  \
        static SENSOR_DEVICE_ATTR(sfp##_num##_tx_disable,  S_IRUGO | S_IWUSR, get_sfp_tx_disable, set_sfp_tx_disable, _num-49);  \
        static SENSOR_DEVICE_ATTR(sfp##_num##_rx_loss,  S_IRUGO, get_sfp_rx_loss, NULL, _num-49);  \
        static SENSOR_DEVICE_ATTR(sfp##_num##_tx_fault,  S_IRUGO, get_sfp_tx_fault, NULL, _num-49) 
#if 0
#define SET_QSFP_ATTR(_num) \
        static SENSOR_DEVICE_ATTR(sfp##_num##_present,  S_IRUGO, get_qsfp_present, NULL, _num-1);  \
        static SENSOR_DEVICE_ATTR(sfp##_num##_reset,  S_IRUGO | S_IWUSR, get_qsfp_reset, set_qsfp_reset, _num-1);  \
        static SENSOR_DEVICE_ATTR(sfp##_num##_lowpower,  S_IRUGO | S_IWUSR, get_qsfp_lowpower, set_qsfp_lowpower, _num-1);  \
        static SENSOR_DEVICE_ATTR(sfp##_num##_modeseln,  S_IRUGO | S_IWUSR, get_qsfp_modeseln, set_qsfp_modeseln, _num-1)
#endif
SET_SFP_ATTR(49);SET_SFP_ATTR(50);SET_SFP_ATTR(51);SET_SFP_ATTR(52);SET_SFP_ATTR(53);SET_SFP_ATTR(54);


static struct attribute *cx206y_48t_cpld_attributes[] = {
    &sensor_dev_attr_cpld_hw_version.dev_attr.attr,
    &sensor_dev_attr_cpld_sw_version.dev_attr.attr,

    &sensor_dev_attr_cpld_allled_ctrl.dev_attr.attr,
    &sensor_dev_attr_serial_led_enable.dev_attr.attr,
    &sensor_dev_attr_sys_led.dev_attr.attr,
    &sensor_dev_attr_pwr_led.dev_attr.attr,
    &sensor_dev_attr_loc_led.dev_attr.attr,
    &sensor_dev_attr_fan_led.dev_attr.attr,
    //&sensor_dev_attr_eeprom_write_enable.dev_attr.attr,
    &sensor_dev_attr_psu_1_present.dev_attr.attr,
    &sensor_dev_attr_psu_2_present.dev_attr.attr,
    &sensor_dev_attr_psu_1_pw_status.dev_attr.attr,
    &sensor_dev_attr_psu_2_pw_status.dev_attr.attr,
    &sensor_dev_attr_psu_1_ac_status.dev_attr.attr,
    &sensor_dev_attr_psu_2_ac_status.dev_attr.attr,

    &sensor_dev_attr_sfp49_present.dev_attr.attr,
    &sensor_dev_attr_sfp49_tx_disable.dev_attr.attr,
    &sensor_dev_attr_sfp49_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp49_tx_fault.dev_attr.attr,

    &sensor_dev_attr_sfp50_present.dev_attr.attr,
    &sensor_dev_attr_sfp50_tx_disable.dev_attr.attr,
    &sensor_dev_attr_sfp50_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp50_tx_fault.dev_attr.attr,

    &sensor_dev_attr_sfp51_present.dev_attr.attr,
    &sensor_dev_attr_sfp51_tx_disable.dev_attr.attr,
    &sensor_dev_attr_sfp51_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp51_tx_fault.dev_attr.attr,

    &sensor_dev_attr_sfp52_present.dev_attr.attr,
    &sensor_dev_attr_sfp52_tx_disable.dev_attr.attr,
    &sensor_dev_attr_sfp52_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp52_tx_fault.dev_attr.attr,

    &sensor_dev_attr_sfp53_present.dev_attr.attr,
    &sensor_dev_attr_sfp53_tx_disable.dev_attr.attr,
    &sensor_dev_attr_sfp53_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp53_tx_fault.dev_attr.attr,

    &sensor_dev_attr_sfp54_present.dev_attr.attr,
    &sensor_dev_attr_sfp54_tx_disable.dev_attr.attr,
    &sensor_dev_attr_sfp54_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp54_tx_fault.dev_attr.attr,
    NULL
};

static const struct attribute_group cx206y_48t_cpld_group = { .attrs = cx206y_48t_cpld_attributes};

static void cx206y_48t_cpld_add_client(struct i2c_client *client)
{
    struct cpld_client_node *node = kzalloc(sizeof(struct cpld_client_node), GFP_KERNEL);
    
    if (!node) {
        dev_dbg(&client->dev, "Can't allocate cpld_client_node (0x%x)\n", client->addr);
        return;
    }
    
    node->client = client;
    
    mutex_lock(&list_lock);
    list_add(&node->list, &cpld_client_list);
    mutex_unlock(&list_lock);
}

static void cx206y_48t_cpld_remove_client(struct i2c_client *client)
{
    struct list_head        *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int found = 0;
    
    mutex_lock(&list_lock);

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);
        
        if (cpld_node->client == client) {
            found = 1;
            break;
        }
    }
    
    if (found) {
        list_del(list_node);
        kfree(cpld_node);
    }
    
    mutex_unlock(&list_lock);
}

static int cx206y_48t_cpld_probe(struct i2c_client *client,
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
        case CPLD_ADDRESS:
            status = sysfs_create_group(&client->dev.kobj, &cx206y_48t_cpld_group);
            break;
        default:
            dev_dbg(&client->dev, "i2c_check_CPLD failed (0x%x)\n", client->addr);
            status = -EIO;
            goto exit;
            break;
    }

    if (status) {
        goto exit;
    }

    dev_info(&client->dev, "chip found\n");
    cx206y_48t_cpld_add_client(client);
    
    return 0; 

exit:
    return status;
}

static int cx206y_48t_cpld_remove(struct i2c_client *client)
{
    switch(client->addr)
    {
        case CPLD_ADDRESS:
            sysfs_remove_group(&client->dev.kobj, &cx206y_48t_cpld_group);
            break;
        default:
            dev_dbg(&client->dev, "i2c_remove_CPLD failed (0x%x)\n", client->addr);
            break;
    }

  
    cx206y_48t_cpld_remove_client(client);
    return 0;
}

static const struct i2c_device_id cx206y_48t_cpld_id[] = {
    { "cx206y_48t_cpld", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, cx206y_48t_cpld_id);

static struct i2c_driver cx206y_48t_cpld_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name = "cx206y_48t_cpld",
    },
    .probe      = cx206y_48t_cpld_probe,
    .remove     = cx206y_48t_cpld_remove,
    .id_table   = cx206y_48t_cpld_id,
    .address_list = normal_i2c,
};

static int __init cx206y_48t_cpld_init(void)
{
    mutex_init(&list_lock);

    return i2c_add_driver(&cx206y_48t_cpld_driver);
}

static void __exit cx206y_48t_cpld_exit(void)
{
    i2c_del_driver(&cx206y_48t_cpld_driver);
}

MODULE_AUTHOR("Wang Lin <wanglin@asterfusion.com>");
MODULE_DESCRIPTION("cx206y_48t_cpld driver");
MODULE_LICENSE("GPL");

module_init(cx206y_48t_cpld_init);
module_exit(cx206y_48t_cpld_exit);
