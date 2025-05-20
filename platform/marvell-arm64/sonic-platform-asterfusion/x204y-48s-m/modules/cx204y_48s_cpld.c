/*
 * A CPLD driver for the cx20y_48s
 *
 * Copyright (C) 2018 asterfusion Corporation.
 * wanglin <wanglin@asterfusion.com>
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
#undef Aster_DEBUG
/*#define Aster_DEBUG*/
#ifdef Aster_DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif /* DEBUG */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/dmi.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>


#define CPLD_ADDRESS                0x30
#define SFPA_ADDRESS                0x70
#define SFPB_ADDRESS                0x71
#define SFPC_ADDRESS                0x72
#define SFPD_ADDRESS                0x73
#define SFPE_ADDRESS                0x74
#define SFPF_ADDRESS                0x75
#define SFPG_ADDRESS                0x76

#define CPLD_VERSION_REG            0x0
#define FAN1_REG                    0x07
#define FAN2_REG                    0x08
#define FAN3_REG                    0x09
#define FAN_LEVEL_REG               0x0a
#define SYS_LED_REG                 0x0b

#define GET_BIT(data, bit, value)   value = (data >> bit) & 0x1
#define SET_BIT(data, bit)          data |= (1 << bit)
#define CLEAR_BIT(data, bit)        data &= ~(1 << bit)

static LIST_HEAD(cpld_client_list);
static struct mutex  list_lock;
/* Addresses scanned for asterfusion_cx204y_48s_cpld
 */
static const unsigned short normal_i2c[] = { CPLD_ADDRESS, SFPA_ADDRESS, SFPB_ADDRESS, SFPC_ADDRESS, SFPD_ADDRESS, SFPE_ADDRESS, SFPF_ADDRESS, SFPG_ADDRESS, I2C_CLIENT_END };

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

int asterfusion_cx204y_48s_cpld_read(unsigned short addr, u8 reg)
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
EXPORT_SYMBOL(asterfusion_cx204y_48s_cpld_read);


int asterfusion_cx204y_48s_cpld_write(unsigned short addr, u8 reg, u8 val)
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
EXPORT_SYMBOL(asterfusion_cx204y_48s_cpld_write);


static ssize_t read_cpld_version(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = CPLD_VERSION_REG;

    data = asterfusion_cx204y_48s_cpld_read(client->addr, reg);

    return sprintf(buf, "%02x\n", data & 0x7);
}


static ssize_t show_fan(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = attr->index;

    data = asterfusion_cx204y_48s_cpld_read(client->addr, reg);

    return sprintf(buf, "%02x\n", data & 0xff);
}

static ssize_t show_fan_level(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 data = 0, reg = FAN_LEVEL_REG;

    data = asterfusion_cx204y_48s_cpld_read(client->addr, reg);

    return sprintf(buf, "%02x\n", data & 0xff);
}

static ssize_t set_fan_level(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);   
    u8 reg = FAN_LEVEL_REG;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, val));

    asterfusion_cx204y_48s_cpld_write(client->addr, reg, val);
    return count;
}

static ssize_t show_sys_led(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    u8 data = 0, reg = SYS_LED_REG;

    data = asterfusion_cx204y_48s_cpld_read(client->addr, reg);

    return sprintf(buf, "%02x\n", data & 0xff);
}

static ssize_t set_sys_led(struct device *dev, struct device_attribute *da,
             const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    u8 reg = SYS_LED_REG;
    long val = 0;

    if (kstrtol(buf, 10, &val))
    {
        return -EINVAL;
    }

    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, reg, val));

    asterfusion_cx204y_48s_cpld_write(client->addr, reg, val);
    return count;
}


static SENSOR_DEVICE_ATTR(cpld_version,  S_IRUGO, read_cpld_version, NULL, 0);
static SENSOR_DEVICE_ATTR(fan1,  S_IRUGO, show_fan, NULL, FAN1_REG);
static SENSOR_DEVICE_ATTR(fan2,  S_IRUGO, show_fan, NULL, FAN2_REG);
static SENSOR_DEVICE_ATTR(fan3,  S_IRUGO, show_fan, NULL, FAN3_REG);
static SENSOR_DEVICE_ATTR(fan_level,  S_IRUGO | S_IWUSR, show_fan_level, set_fan_level, 0);
static SENSOR_DEVICE_ATTR(sys_led, S_IRUGO | S_IWUSR, show_sys_led, set_sys_led, 0);

static struct attribute *asterfusion_cx204y_48s_cpld_attributes[] = {
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    &sensor_dev_attr_fan1.dev_attr.attr,
    &sensor_dev_attr_fan2.dev_attr.attr,
    &sensor_dev_attr_fan3.dev_attr.attr,
    &sensor_dev_attr_fan_level.dev_attr.attr,
    &sensor_dev_attr_sys_led.dev_attr.attr,
    NULL
};

static const struct attribute_group asterfusion_cx204y_48s_cpld_group = { .attrs = asterfusion_cx204y_48s_cpld_attributes};

static void asterfusion_cx204y_48s_cpld_add_client(struct i2c_client *client)
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

static void asterfusion_cx204y_48s_cpld_remove_client(struct i2c_client *client)
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

static int asterfusion_cx204y_48s_cpld_probe(struct i2c_client *client,
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
            status = sysfs_create_group(&client->dev.kobj, &asterfusion_cx204y_48s_cpld_group);
            break;
        case SFPA_ADDRESS:
        case SFPB_ADDRESS:
        case SFPC_ADDRESS:
        case SFPD_ADDRESS:
        case SFPE_ADDRESS:
        case SFPF_ADDRESS:
        case SFPG_ADDRESS:
            status = 0;
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
    asterfusion_cx204y_48s_cpld_add_client(client);
    
    return 0; 

exit:
    return status;
}

static int asterfusion_cx204y_48s_cpld_remove(struct i2c_client *client)
{
    switch(client->addr)
    {
        case CPLD_ADDRESS:
            sysfs_remove_group(&client->dev.kobj, &asterfusion_cx204y_48s_cpld_group);
            break;
        case SFPA_ADDRESS:
        case SFPB_ADDRESS:
        case SFPC_ADDRESS:
        case SFPD_ADDRESS:
        case SFPE_ADDRESS:
        case SFPF_ADDRESS:
        case SFPG_ADDRESS:
            break;

        default:
            dev_dbg(&client->dev, "i2c_remove_CPLD failed (0x%x)\n", client->addr);
            break;
    }
  
    asterfusion_cx204y_48s_cpld_remove_client(client);  
    return 0;
}

static const struct i2c_device_id asterfusion_cx204y_48s_cpld_id[] = {
    { "cx204y_48s_cpld", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, asterfusion_cx204y_48s_cpld_id);

static struct i2c_driver asterfusion_cx204y_48s_cpld_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name = "asterfusion_cx204y_48s_cpld",
    },
    .probe      = asterfusion_cx204y_48s_cpld_probe,
    .remove     = asterfusion_cx204y_48s_cpld_remove,
    .id_table   = asterfusion_cx204y_48s_cpld_id,
    .address_list = normal_i2c,
};

static int __init asterfusion_cx204y_48s_cpld_init(void)
{
    mutex_init(&list_lock);

    return i2c_add_driver(&asterfusion_cx204y_48s_cpld_driver);
}

static void __exit asterfusion_cx204y_48s_cpld_exit(void)
{
    i2c_del_driver(&asterfusion_cx204y_48s_cpld_driver);
}

MODULE_AUTHOR("Wang Lin <wanglin@asterfusion.com>");
MODULE_DESCRIPTION("cx204y_48s_cpld driver");
MODULE_LICENSE("GPL");

module_init(asterfusion_cx204y_48s_cpld_init);
module_exit(asterfusion_cx204y_48s_cpld_exit);
