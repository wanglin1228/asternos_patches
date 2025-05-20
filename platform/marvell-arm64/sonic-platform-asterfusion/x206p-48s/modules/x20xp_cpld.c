/*
 * A CPLD driver for the x20xp cpld
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


/*pca9548 address*/
#define PCA9548_0X70                0x70
#define PCA9548_0X71                0x71
#define PCA9548_0X72                0x72
#define PCA9548_0X73                0x73
#define PCA9548_0X74                0x74
#define PCA9548_0X75                0x75
#define PCA9548_0X76                0x76

#define CPLD_ADDRESS                0x40
/* struct i2c_sysfs_attributes */
#define CPLD_VER                    0x00
#define BOARD_VER                   0x01
#define TEST_REG                    0x02
#define ADC1_STAT                   0x03
#define ADC2_STAT                   0x04
#define INTERRUPT1_STAT             0x05
#define INTERRUPT2_STAT             0x06
#define BOARD_STAT                  0x07
#define RST1_CTL                    0x08
#define RST2_CTL                    0x09
#define SYS_LED                     0x0A
#define CPLD_CTL1                   0x0B
#define FAN1_OUTLET_RPM             0x0C
#define FAN1_INLET_RPM              0x0D
#define FAN2_OUTLET_RPM             0x0E
#define FAN2_INLET_RPM              0x0F
#define FAN3_OUTLET_RPM             0x10
#define FAN3_INLET_RPM              0x11
#define FAN_STAT                    0x12
#define FAN_CTL1                    0x13
#define FAN_CTL2                    0x14
#define FAN_BOARD_SEL               0x15
#define FAN_LM75_R                  0x16
#define FAN_LM75_L                  0x17
#define CPU_LM75                    0x18
#define SWITCH_LM75                 0x19
#define I2C_BUS_STAT                0x1A
#define LOC_LED                     0x1B
#define PSU_STAT                    0x1C
#define FAN_LEVEL                   0x1D
#define HW_RESET                    0x1E
/* end of struct i2c_sysfs_attributes */

/*client reset */
#define SFP_SCL_BASE                0x0

#define EEPROM_WP_CTRL_BIT  7
#define OVER_TEMP_CTRL_BIT  1
#define WDT_CTRL_BIT        0

#define FAN3_STATUS_BIT     5
#define FAN2_STATUS_BIT     4
#define FAN1_STATUS_BIT     3
#define FAN3_PRESNET_BIT    2
#define FAN2_PRESNET_BIT    1
#define FAN1_PRESNET_BIT    0

#define PSU2_POWER_BIT      5
#define PSU1_POWER_BIT      4
#define PSU2_ALERT_BIT      3
#define PSU1_ALERT_BIT      2
#define PSU2_PRESNET_BIT    1
#define PSU1_PRESNET_BIT    0

#define LED_GREEN_BLINK     0
#define LED_GREEN           1

#define FAN_CPLD_RST_BIT    5
#define IO_MUX_RST_BIT      4
#define LED_STR_RST_BIT     3
#define PCA9548_RST_BIT     2
#define GE_PHY_RST_BIT      1
#define SWITCH_RST_BIT      0

#define QSFP28_6_RST_BIT    5
#define QSFP28_5_RST_BIT    4
#define QSFP28_4_RST_BIT    3
#define QSFP28_3_RST_BIT    2
#define QSFP28_2_RST_BIT    1
#define QSFP28_1_RST_BIT    0


#define GET_BIT(data, bit, value)   value = (data >> bit) & 0x1
#define SET_BIT(data, bit)          data |= (1 << bit)
#define CLEAR_BIT(data, bit)        data &= ~(1 << bit)

static LIST_HEAD(cpld_client_list);
static struct mutex  list_lock;
/* Addresses scanned for asterfusion_x20xp_cpld
 */
static const unsigned short normal_i2c[] = { CPLD_ADDRESS, PCA9548_0X70, PCA9548_0X71, PCA9548_0X72, PCA9548_0X73, PCA9548_0X74, PCA9548_0X75, PCA9548_0X76, I2C_CLIENT_END };

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

int asterfusion_x20xp_cpld_read(unsigned short addr, u8 reg)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int data = -EPERM;

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);
        
        if (cpld_node->client->addr == addr) {
            data = i2c_smbus_read_byte_data(cpld_node->client, reg);
            DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, addr, reg, data));
            break;
        }
    }

    return data;
}
EXPORT_SYMBOL(asterfusion_x20xp_cpld_read);

int asterfusion_x20xp_cpld_write(unsigned short addr, u8 reg, u8 val)
{
    struct list_head   *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EIO;

    list_for_each(list_node, &cpld_client_list)
    {
        cpld_node = list_entry(list_node, struct cpld_client_node, list);

        if (cpld_node->client->addr == addr) {
            ret = i2c_smbus_write_byte_data(cpld_node->client, reg, val);
            DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, addr, reg, val));
            break;
        }
    }

    return ret;
}
EXPORT_SYMBOL(asterfusion_x20xp_cpld_write);

void asterfusion_x20xp_read_lock(void)
{
    mutex_lock(&list_lock);
}
EXPORT_SYMBOL(asterfusion_x20xp_read_lock);

void asterfusion_x20xp_read_unlock(void)
{
    mutex_unlock(&list_lock);
}
EXPORT_SYMBOL(asterfusion_x20xp_read_unlock);

int asterfusion_x20xp_cpld_reset(void)
{
    int sfpBase;

    for(sfpBase = PCA9548_0X70; sfpBase < PCA9548_0X76 + 1; sfpBase++)
    { 
        asterfusion_x20xp_cpld_write(sfpBase, SFP_SCL_BASE, 0);
    }

    return 0;
}
EXPORT_SYMBOL(asterfusion_x20xp_cpld_reset);


int asterfusion_x20xp_cpld_read_on_lock(unsigned short addr, u8 reg)
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


int asterfusion_x20xp_cpld_write_on_lock(unsigned short addr, u8 reg, u8 val)
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

/* i2c-0 function */

static ssize_t cpld_byte_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u32 status = -EPERM;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    sprintf(buf, "");

    if (CPLD_VER == attr->index || BOARD_VER == attr->index)
    {
        status = asterfusion_x20xp_cpld_read_on_lock(client->addr, attr->index);
    }

    if (CPLD_VER == attr->index)
    {
        sprintf(buf, "%sCPLD version", buf);
    }
    else if (BOARD_VER == attr->index)
    {
       sprintf(buf, "%sBoard version", buf);
    }
    return sprintf(buf, "%s is %02x\n", buf, status);
}

static ssize_t sys_adc1_status_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u32 status = -EPERM;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 i;
    u16 dc_status [8] = {0};
    u8 mask = 0x1;

    sprintf(buf, "");
    if (attr->index == ADC1_STAT)
    {
        status = asterfusion_x20xp_cpld_read_on_lock(client->addr, attr->index);
        for (i = 0; i < 8; i++)
        {
            dc_status[i] = status & mask;
            mask = mask << 1;
        }
        sprintf(buf, "%svcc1v2_pg       is %s\n", buf, dc_status[0]?"normal":"abnormal");
        sprintf(buf, "%svcc5v_pg        is %s\n", buf, dc_status[1]?"normal":"abnormal");
        sprintf(buf, "%svcc1v8_pg       is %s\n", buf, dc_status[2]?"normal":"abnormal");
        sprintf(buf, "%svcc2v5_pg       is %s\n", buf, dc_status[3]?"normal":"abnormal");
        sprintf(buf, "%svcc_0p82v       is %s\n", buf, dc_status[4]?"normal":"abnormal");
        sprintf(buf, "%svtt_ddr         is %s\n", buf, dc_status[5]?"normal":"abnormal");
        sprintf(buf, "%svcc1v8_sw_pg    is %s\n", buf, dc_status[6]?"normal":"abnormal");
        sprintf(buf, "%spower_good_all  is %s\n", buf, dc_status[7]?"normal":"abnormal");
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t sys_adc2_status_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u32 status = -EPERM;
    u8 i;
    u16 dc_status [6] = {0};
    u8 mask = 0x1;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);


    sprintf(buf, "");
    if (attr->index == ADC2_STAT)
    {
        status = asterfusion_x20xp_cpld_read_on_lock(client->addr, attr->index);
        for (i = 0; i < 6; i++)
        {
            dc_status[i] = status & mask;
            mask = mask << 1;
        }
        sprintf(buf, "%ssd_avdd_pg      is %s\n", buf, dc_status[0]?"normal":"abnormal");
        sprintf(buf, "%svcc3v3_pg       is %s\n", buf, dc_status[1]?"normal":"abnormal");
        sprintf(buf, "%svdd_core_pg     is %s\n", buf, dc_status[2]?"normal":"abnormal");
        sprintf(buf, "%ssd_avdd_alt_n   is %s\n", buf, dc_status[3]?"normal":"abnormal");
        sprintf(buf, "%svcc3v3_alt_n    is %s\n", buf, dc_status[4]?"normal":"abnormal");
        sprintf(buf, "%svdd_core_alt_n  is %s\n", buf, dc_status[5]?"normal":"abnormal");
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t rst1_ctl_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u32 status = -EPERM;
    u8 value = -EPERM;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    status = asterfusion_x20xp_cpld_read_on_lock(client->addr, RST1_CTL);
    GET_BIT(status, attr->index, value);

    sprintf(buf, "");
    switch(attr->index)
    {
        case FAN_CPLD_RST_BIT:
            sprintf(buf, "%sRESET_FAN_CPLD status =  0x%x\n", buf, value);
            break;
        case IO_MUX_RST_BIT:
            sprintf(buf, "%sRESET_O_MUX status =  0x%x\n", buf, value);
            break;
        case LED_STR_RST_BIT:
            sprintf(buf, "%sRESET_LED_STR status =  0x%x\n", buf, value);
            break;
        case PCA9548_RST_BIT:
            sprintf(buf, "%sRESET_PCA9548 status =  0x%x\n", buf, value);
            break;
        case GE_PHY_RST_BIT:
            sprintf(buf, "%sRESET_GE_PHY status =  0x%x\n", buf, value);
            break;
        case SWITCH_RST_BIT:
            sprintf(buf, "%sRESET_SWITCH status =  0x%x\n", buf, value);
            break;
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t rst1_ctl_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u32 status = -EPERM;
    u16 val = simple_strtol(buf, NULL, 10);
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    status = asterfusion_x20xp_cpld_read_on_lock(client->addr, RST1_CTL);

    if(val)
        SET_BIT(status, (attr->index % 8));
    else
        CLEAR_BIT(status, (attr->index % 8));

    status = asterfusion_x20xp_cpld_write_on_lock(client->addr, RST1_CTL, status);
    return count;
}

static ssize_t rst2_ctl_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u32 status = -EPERM;
    u8 value = -EPERM;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    status = asterfusion_x20xp_cpld_read_on_lock(client->addr, RST2_CTL);
    GET_BIT(status, attr->index, value);

    sprintf(buf, "");
    switch(attr->index)
    {
        case QSFP28_6_RST_BIT:
            sprintf(buf, "%sRESET_QSFP28_6 status =  0x%x\n", buf, value);
            break;
        case QSFP28_5_RST_BIT:
            sprintf(buf, "%sRESET_QSFP28_5 status =  0x%x\n", buf, value);
            break;
        case QSFP28_4_RST_BIT:
            sprintf(buf, "%sRESET_QSFP28_4 status =  0x%x\n", buf, value);
            break;
        case QSFP28_3_RST_BIT:
            sprintf(buf, "%sRESET_QSFP28_3 status =  0x%x\n", buf, value);
            break;
        case QSFP28_2_RST_BIT:
            sprintf(buf, "%sRESET_QSFP28_2 status =  0x%x\n", buf, value);
            break;
        case QSFP28_1_RST_BIT:
            sprintf(buf, "%sRESET_QSFP28_1 status =  0x%x\n", buf, value);
            break;
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t rst2_ctl_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u32 status = -EPERM;
    u16 val = simple_strtol(buf, NULL, 10);
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    status = asterfusion_x20xp_cpld_read_on_lock(client->addr, RST2_CTL);
 
    if(val)
        SET_BIT(status, (attr->index % 8));
    else
        CLEAR_BIT(status, (attr->index % 8));

    status = asterfusion_x20xp_cpld_write_on_lock(client->addr, RST2_CTL, status);
    return count;
}

static ssize_t cpld_ctrl1_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u32 status = -EPERM;
    u8 value = -EPERM;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    status = asterfusion_x20xp_cpld_read_on_lock(client->addr, CPLD_CTL1);
    GET_BIT(status, attr->index, value);

    sprintf(buf, "");
    switch(attr->index)
    {
        case EEPROM_WP_CTRL_BIT:
            sprintf(buf, "%sEEPROM_WP status =  0x%x\n", buf, value);
            break;
        case OVER_TEMP_CTRL_BIT:
            sprintf(buf, "%sOVER_TEMP status =  0x%x\n", buf, value);
            break;
        case WDT_CTRL_BIT:
            sprintf(buf, "%sWDT status =  0x%x\n", buf, value);
            break;
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t cpld_ctrl1_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u32 status = -EPERM;
    u16 val = simple_strtol(buf, NULL, 10);
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    status = asterfusion_x20xp_cpld_read_on_lock(client->addr, CPLD_CTL1);

    if(val)
        SET_BIT(status, (attr->index % 8));
    else
        CLEAR_BIT(status, (attr->index % 8));

    status = asterfusion_x20xp_cpld_write_on_lock(client->addr, CPLD_CTL1, status);
    return count;
}

static ssize_t fan_rpm_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u32 status = -EPERM;

    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    sprintf(buf, "");

    if (FAN1_OUTLET_RPM == attr->index || FAN2_OUTLET_RPM == attr->index || FAN3_OUTLET_RPM == attr->index
        || FAN1_INLET_RPM == attr->index || FAN2_INLET_RPM == attr->index || FAN3_INLET_RPM == attr->index)
    {
        status = asterfusion_x20xp_cpld_read_on_lock(client->addr, attr->index);
        sprintf(buf, "%s%d", buf, status);
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t fan_stat_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u32 status = -EPERM;
    u8 value = -EPERM;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    sprintf(buf, "");

    status = asterfusion_x20xp_cpld_read_on_lock(client->addr, FAN_STAT);
    GET_BIT(status, attr->index, value);

    return sprintf(buf, "%s%d\n", buf, value);
}

static ssize_t fan_mode_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u32 status = -EPERM;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    sprintf(buf, "");

    if (FAN_CTL1 == attr->index)
    {
        status = asterfusion_x20xp_cpld_read_on_lock(client->addr, attr->index);
        sprintf(buf, "%s%d", buf, status & 0x1);
    }

    return sprintf(buf, "%s\n", buf);
}

static ssize_t fan_mode_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u32 status = -EPERM;
    u16 val = simple_strtol(buf, NULL, 10);
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    if (FAN_CTL1 == attr->index)
    {
        status = asterfusion_x20xp_cpld_write_on_lock(client->addr, attr->index, val & 0x1);
    }

    return count;
}

static ssize_t fan_board_sel_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u32 status = -EPERM;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    sprintf(buf, "");

    if (FAN_BOARD_SEL == attr->index)
    {
        status = asterfusion_x20xp_cpld_read_on_lock(client->addr, attr->index);
        sprintf(buf, "%sfan_board_i2c_sel is %d", buf, status & 0x1);
    }

    return sprintf(buf, "%s\n", buf);
}

static ssize_t fan_board_sel_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u32 status = -EPERM;
    u16 val = simple_strtol(buf, NULL, 10);
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    if (FAN_BOARD_SEL == attr->index)
    {
        status = asterfusion_x20xp_cpld_write_on_lock(client->addr, attr->index, val & 0x1);
    }

    return count;
}

static ssize_t fan_speed_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u32 status = -EPERM;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    sprintf(buf, "");

    if (FAN_CTL2 == attr->index)
    {
        status = asterfusion_x20xp_cpld_read_on_lock(client->addr, attr->index);
        sprintf(buf, "%sfan_mode_level is %d", buf, status % 8);
    }

    return sprintf(buf, "%s\n", buf);
}

static ssize_t fan_speed_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u32 status = -EPERM;
    u16 val = simple_strtol(buf, NULL, 10);
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    if (FAN_CTL2 == attr->index)
    {
        status = asterfusion_x20xp_cpld_write_on_lock(client->addr, attr->index, val % 8);
    }

    return count;
}

static ssize_t themal_temp_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u32 status = -EPERM;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    sprintf(buf, "");

    if (FAN_LM75_R == attr->index || FAN_LM75_L == attr->index || SWITCH_LM75 == attr->index || CPU_LM75 == attr->index)
    {
        status = asterfusion_x20xp_cpld_read_on_lock(client->addr, attr->index);
        sprintf(buf, "%s%d", buf, status);
    }

    return sprintf(buf, "%s\n", buf);
}

static ssize_t sys_led_ctrl_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status = -EPERM;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    sprintf(buf, "SYS LED is set to: ");
    if (SYS_LED == attr->index)
    {
        status = asterfusion_x20xp_cpld_read_on_lock(client->addr, SYS_LED); //to get register 0x30 0x13
        status &= 0x1;

        if(status == LED_GREEN_BLINK)
        {
            sprintf(buf, "%sgreen and blink\n", buf);
        }
        else if(status == LED_GREEN)
        {
            sprintf(buf, "%sgreen\n", buf);
        }
    }

    return sprintf(buf, "%s", buf);
}

static ssize_t sys_led_ctrl_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u32 status = -EPERM;
    u16 val = simple_strtol(buf, NULL, 10);
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    if (SYS_LED == attr->index)
    {
        status = asterfusion_x20xp_cpld_write_on_lock(client->addr, attr->index, val & 0x1);
    }
    return count;
}

static ssize_t loc_led_ctrl_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status = -EPERM;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    sprintf(buf, "LOC LED is set to: ");
    if (LOC_LED == attr->index)
    {
        status = asterfusion_x20xp_cpld_read_on_lock(client->addr, LOC_LED); //to get register 0x30 0x13
        status &= 0x1;

        if(status == 0)
        {
            sprintf(buf, "%soff\n", buf);
        }
        else if(status == 1)
        {
            sprintf(buf, "%son\n", buf);
        }
    }

    return sprintf(buf, "%s", buf);
}

static ssize_t loc_led_ctrl_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u32 status = -EPERM;
    u16 val = simple_strtol(buf, NULL, 10);
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    if (LOC_LED == attr->index)
    {
        status = asterfusion_x20xp_cpld_write_on_lock(client->addr, attr->index, val & 0x1);
    }
    return count;
}

static ssize_t psu_stat_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u32 status = -EPERM;
    u8 value = -EPERM;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    sprintf(buf, "");

    status = asterfusion_x20xp_cpld_read_on_lock(client->addr, PSU_STAT);
    GET_BIT(status, attr->index, value);

    // TODO why did not split to get value
    return sprintf(buf, "%s%d\n", buf, value);
}

static ssize_t hw_reset_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status = -EPERM;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    sprintf(buf, "LOC LED is set to: ");
    if (HW_RESET == attr->index)
    {
        status = asterfusion_x20xp_cpld_read_on_lock(client->addr, HW_RESET); //to get register 0x30 0x13
        status &= 0x1;

        if(status == 0)
        {
            sprintf(buf, "%sset\n", buf);
        }
        else if(status == 1)
        {
            sprintf(buf, "%snot set\n", buf);
        }
    }

    return sprintf(buf, "%s", buf);
}

static ssize_t hw_reset_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u32 status = -EPERM;
    u16 val = simple_strtol(buf, NULL, 10);
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    if (HW_RESET == attr->index)
    {
        status = asterfusion_x20xp_cpld_write_on_lock(client->addr, attr->index, val & 0x1);
    }
    return count;
}

/* end of function */


/* sysfs attributes for SENSOR_DEVICE_ATTR */
/* i2c-0 */
/*CPLD 0X40*/

static SENSOR_DEVICE_ATTR(cpld_version      , S_IRUGO           , cpld_byte_get     , NULL                , CPLD_VER);    //0x00
static SENSOR_DEVICE_ATTR(board_version     , S_IRUGO           , cpld_byte_get     , NULL                , BOARD_VER);   //0x01

static SENSOR_DEVICE_ATTR(adc1_status        , S_IRUGO           , sys_adc1_status_get, NULL              , ADC1_STAT);//0x03
static SENSOR_DEVICE_ATTR(adc2_status        , S_IRUGO           , sys_adc2_status_get, NULL              , ADC2_STAT);//0x04

static SENSOR_DEVICE_ATTR(fan_cpld_rst      , S_IRUGO | S_IWUSR , rst1_ctl_get       , rst1_ctl_set       , FAN_CPLD_RST_BIT);//0x08
static SENSOR_DEVICE_ATTR(io_mux_rst        , S_IRUGO | S_IWUSR , rst1_ctl_get       , rst1_ctl_set       , IO_MUX_RST_BIT);
static SENSOR_DEVICE_ATTR(led_str_rst       , S_IRUGO | S_IWUSR , rst1_ctl_get       , rst1_ctl_set       , LED_STR_RST_BIT);
static SENSOR_DEVICE_ATTR(pca9548_rst       , S_IRUGO | S_IWUSR , rst1_ctl_get       , rst1_ctl_set       , PCA9548_RST_BIT);
static SENSOR_DEVICE_ATTR(ge_phy_rst        , S_IRUGO | S_IWUSR , rst1_ctl_get       , rst1_ctl_set       , GE_PHY_RST_BIT);
static SENSOR_DEVICE_ATTR(switch_rst        , S_IRUGO | S_IWUSR , rst1_ctl_get       , rst1_ctl_set       , SWITCH_RST_BIT);

static SENSOR_DEVICE_ATTR(qsfp28_6_rst      , S_IRUGO | S_IWUSR , rst2_ctl_get       , rst2_ctl_set       , QSFP28_6_RST_BIT);//0x09
static SENSOR_DEVICE_ATTR(qsfp28_5_rst      , S_IRUGO | S_IWUSR , rst2_ctl_get       , rst2_ctl_set       , QSFP28_5_RST_BIT);
static SENSOR_DEVICE_ATTR(qsfp28_4_rst      , S_IRUGO | S_IWUSR , rst2_ctl_get       , rst2_ctl_set       , QSFP28_4_RST_BIT);
static SENSOR_DEVICE_ATTR(qsfp28_3_rst      , S_IRUGO | S_IWUSR , rst2_ctl_get       , rst2_ctl_set       , QSFP28_3_RST_BIT);
static SENSOR_DEVICE_ATTR(qsfp28_2_rst      , S_IRUGO | S_IWUSR , rst2_ctl_get       , rst2_ctl_set       , QSFP28_2_RST_BIT);
static SENSOR_DEVICE_ATTR(qsfp28_1_rst      , S_IRUGO | S_IWUSR , rst2_ctl_get       , rst2_ctl_set       , QSFP28_1_RST_BIT);

static SENSOR_DEVICE_ATTR(led_sys           , S_IRUGO | S_IWUSR , sys_led_ctrl_get  , sys_led_ctrl_set  , SYS_LED);//0x0A

static SENSOR_DEVICE_ATTR(eeprom_wp_ctrl    , S_IRUGO | S_IWUSR , cpld_ctrl1_get    , cpld_ctrl1_set    , EEPROM_WP_CTRL_BIT);//0x0B
static SENSOR_DEVICE_ATTR(over_temp_ctrl    , S_IRUGO | S_IWUSR , cpld_ctrl1_get    , cpld_ctrl1_set    , OVER_TEMP_CTRL_BIT);
static SENSOR_DEVICE_ATTR(wdt_ctrl          , S_IRUGO | S_IWUSR , cpld_ctrl1_get    , cpld_ctrl1_set    , WDT_CTRL_BIT);

static SENSOR_DEVICE_ATTR(fan1_outlet_rpm   , S_IRUGO           , fan_rpm_get       , NULL              , FAN1_OUTLET_RPM);//0x0C
static SENSOR_DEVICE_ATTR(fan1_inlet_rpm    , S_IRUGO           , fan_rpm_get       , NULL              , FAN1_INLET_RPM);//0x0D
static SENSOR_DEVICE_ATTR(fan2_outlet_rpm   , S_IRUGO           , fan_rpm_get       , NULL              , FAN2_OUTLET_RPM);//0xE
static SENSOR_DEVICE_ATTR(fan2_inlet_rpm    , S_IRUGO           , fan_rpm_get       , NULL              , FAN2_INLET_RPM);//0x0F
static SENSOR_DEVICE_ATTR(fan3_outlet_rpm   , S_IRUGO           , fan_rpm_get       , NULL              , FAN3_OUTLET_RPM);//0x10
static SENSOR_DEVICE_ATTR(fan3_inlet_rpm    , S_IRUGO           , fan_rpm_get       , NULL              , FAN3_INLET_RPM);//0x11

static SENSOR_DEVICE_ATTR(fan1_status       , S_IRUGO           , fan_stat_get      , NULL              , FAN1_STATUS_BIT);//0x12
static SENSOR_DEVICE_ATTR(fan2_status       , S_IRUGO           , fan_stat_get      , NULL              , FAN2_STATUS_BIT);
static SENSOR_DEVICE_ATTR(fan3_status       , S_IRUGO           , fan_stat_get      , NULL              , FAN3_STATUS_BIT);
static SENSOR_DEVICE_ATTR(fan1_present      , S_IRUGO           , fan_stat_get      , NULL              , FAN1_PRESNET_BIT);
static SENSOR_DEVICE_ATTR(fan2_present      , S_IRUGO           , fan_stat_get      , NULL              , FAN2_PRESNET_BIT);
static SENSOR_DEVICE_ATTR(fan3_present      , S_IRUGO           , fan_stat_get      , NULL              , FAN3_PRESNET_BIT);

static SENSOR_DEVICE_ATTR(fan_mode          , S_IRUGO | S_IWUSR , fan_mode_get      , fan_mode_set      , FAN_CTL1);//0x13
static SENSOR_DEVICE_ATTR(fan_speed         , S_IRUGO | S_IWUSR , fan_speed_get     , fan_speed_set     , FAN_CTL2);//0x14
static SENSOR_DEVICE_ATTR(fan_board_sel     , S_IRUGO | S_IWUSR , fan_board_sel_get , fan_board_sel_set , FAN_BOARD_SEL);//0x15

static SENSOR_DEVICE_ATTR(fan_lm75_right    , S_IRUGO           , themal_temp_get   , NULL              , FAN_LM75_R);//0x16
static SENSOR_DEVICE_ATTR(fan_lm75_left     , S_IRUGO           , themal_temp_get   , NULL              , FAN_LM75_L);//0x17
static SENSOR_DEVICE_ATTR(cpu_lm75          , S_IRUGO           , themal_temp_get   , NULL              , CPU_LM75);//0x18
static SENSOR_DEVICE_ATTR(switch_lm75       , S_IRUGO           , themal_temp_get   , NULL              , SWITCH_LM75);//0x19

static SENSOR_DEVICE_ATTR(led_loc           , S_IRUGO | S_IWUSR , loc_led_ctrl_get  , loc_led_ctrl_set  , LOC_LED);//0x1B

static SENSOR_DEVICE_ATTR(psu1_present      , S_IRUGO           , psu_stat_get       , NULL              , PSU1_PRESNET_BIT);//0x1C
static SENSOR_DEVICE_ATTR(psu2_present      , S_IRUGO           , psu_stat_get       , NULL              , PSU2_PRESNET_BIT);
static SENSOR_DEVICE_ATTR(psu1_status       , S_IRUGO           , psu_stat_get       , NULL              , PSU1_ALERT_BIT);
static SENSOR_DEVICE_ATTR(psu2_status       , S_IRUGO           , psu_stat_get       , NULL              , PSU2_ALERT_BIT);
static SENSOR_DEVICE_ATTR(psu1_power        , S_IRUGO           , psu_stat_get       , NULL              , PSU1_POWER_BIT);
static SENSOR_DEVICE_ATTR(psu2_power        , S_IRUGO           , psu_stat_get       , NULL              , PSU2_POWER_BIT);

static SENSOR_DEVICE_ATTR(hw_reset          , S_IRUGO           , hw_reset_get       , hw_reset_set      , HW_RESET);//0x1E

/* end of sysfs attributes for SENSOR_DEVICE_ATTR */



/* sysfs attributes for hwmon */
/* i2c-0 */
static struct attribute *X20XP_SYS_attributes[] =
{
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    &sensor_dev_attr_board_version.dev_attr.attr,
    &sensor_dev_attr_adc1_status.dev_attr.attr,
    &sensor_dev_attr_adc2_status.dev_attr.attr,
    &sensor_dev_attr_eeprom_wp_ctrl.dev_attr.attr,
    &sensor_dev_attr_over_temp_ctrl.dev_attr.attr,
    &sensor_dev_attr_wdt_ctrl.dev_attr.attr,
    NULL
};

static struct attribute *X20XP_Reset_attributes[] =
{
    &sensor_dev_attr_fan_cpld_rst.dev_attr.attr,
    &sensor_dev_attr_io_mux_rst.dev_attr.attr,
    &sensor_dev_attr_led_str_rst.dev_attr.attr,
    &sensor_dev_attr_pca9548_rst.dev_attr.attr,
    &sensor_dev_attr_ge_phy_rst.dev_attr.attr,
    &sensor_dev_attr_switch_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp28_6_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp28_5_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp28_4_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp28_3_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp28_2_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp28_1_rst.dev_attr.attr,
    &sensor_dev_attr_hw_reset.dev_attr.attr,
    NULL
};

static struct attribute *X20XP_FAN_attributes[] = {
    &sensor_dev_attr_fan1_outlet_rpm.dev_attr.attr,
    &sensor_dev_attr_fan1_inlet_rpm.dev_attr.attr,
    &sensor_dev_attr_fan2_outlet_rpm.dev_attr.attr,
    &sensor_dev_attr_fan2_inlet_rpm.dev_attr.attr,
    &sensor_dev_attr_fan3_outlet_rpm.dev_attr.attr,
    &sensor_dev_attr_fan3_inlet_rpm.dev_attr.attr,
    &sensor_dev_attr_fan1_status.dev_attr.attr,
    &sensor_dev_attr_fan2_status.dev_attr.attr,
    &sensor_dev_attr_fan3_status.dev_attr.attr,
    &sensor_dev_attr_fan1_present.dev_attr.attr,
    &sensor_dev_attr_fan2_present.dev_attr.attr,
    &sensor_dev_attr_fan3_present.dev_attr.attr,
    &sensor_dev_attr_fan_mode.dev_attr.attr,
    &sensor_dev_attr_fan_speed.dev_attr.attr,
    &sensor_dev_attr_fan_board_sel.dev_attr.attr,
    NULL
};

static struct attribute *X20XP_Sensor_attributes[] =
{
    &sensor_dev_attr_fan_lm75_right.dev_attr.attr,
    &sensor_dev_attr_fan_lm75_left.dev_attr.attr,
    &sensor_dev_attr_cpu_lm75.dev_attr.attr,
    &sensor_dev_attr_switch_lm75.dev_attr.attr,
    NULL
};

static struct attribute *X20XP_Led_attributes[] =
{
    &sensor_dev_attr_led_sys.dev_attr.attr,
    &sensor_dev_attr_led_loc.dev_attr.attr,
    NULL
};

static struct attribute *X20XP_PSU_attributes[] =
{
    &sensor_dev_attr_psu1_present.dev_attr.attr,
    &sensor_dev_attr_psu2_present.dev_attr.attr,
    &sensor_dev_attr_psu1_status.dev_attr.attr,
    &sensor_dev_attr_psu2_status.dev_attr.attr,
    &sensor_dev_attr_psu1_power.dev_attr.attr,
    &sensor_dev_attr_psu2_power.dev_attr.attr,
    NULL
};

/* end of sysfs attributes for hwmon */

/* struct attribute_group */
static const struct attribute_group X20XP_SYS_group =
{
    .name  = "X20XP_SYS",
    .attrs = X20XP_SYS_attributes,
};

static const struct attribute_group X20XP_Reset_group =
{
    .name  = "X20XP_Reset",
    .attrs = X20XP_Reset_attributes,
};

static const struct attribute_group X20XP_Sensor_group =
{
    .name  = "X20XP_Sensor",
    .attrs = X20XP_Sensor_attributes,
};

static const struct attribute_group X20XP_Led_group =
{
    .name  = "X20XP_Led",
    .attrs = X20XP_Led_attributes,
};

static const struct attribute_group X20XP_FAN_group =
{
    .name  = "X20XP_FAN",
    .attrs = X20XP_FAN_attributes,
};

static const struct attribute_group X20XP_PSU_group =
{
    .name  = "X20XP_PSU",
    .attrs = X20XP_PSU_attributes,
};

/* end of struct attribute_group */


static void asterfusion_x20xp_cpld_add_client(struct i2c_client *client)
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

static void asterfusion_x20xp_cpld_remove_client(struct i2c_client *client)
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

static int asterfusion_x20xp_cpld_probe(struct i2c_client *client,
            const struct i2c_device_id *dev_id)
{  
    int status;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA))
    {
        dev_dbg(&client->dev, "i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    /* Register sysfs hooks */
    switch(client->addr)
    {
        case CPLD_ADDRESS:
            /* TODO add status error deal */
            status = sysfs_create_group(&client->dev.kobj, &X20XP_SYS_group);
            status = sysfs_create_group(&client->dev.kobj, &X20XP_Reset_group);
            status = sysfs_create_group(&client->dev.kobj, &X20XP_Sensor_group);
            status = sysfs_create_group(&client->dev.kobj, &X20XP_Led_group);
            status = sysfs_create_group(&client->dev.kobj, &X20XP_FAN_group);
            status = sysfs_create_group(&client->dev.kobj, &X20XP_PSU_group);
            break;
        case PCA9548_0X70:
        case PCA9548_0X71:
        case PCA9548_0X72:
        case PCA9548_0X73:
        case PCA9548_0X74:
        case PCA9548_0X75:
        case PCA9548_0X76:
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
    asterfusion_x20xp_cpld_add_client(client);

    return 0; 

exit:
    return status;
}

static int asterfusion_x20xp_cpld_remove(struct i2c_client *client)
{
    switch(client->addr)
    {
        case CPLD_ADDRESS:
            sysfs_remove_group(&client->dev.kobj, &X20XP_SYS_group);
            sysfs_remove_group(&client->dev.kobj, &X20XP_Reset_group);
            sysfs_remove_group(&client->dev.kobj, &X20XP_Sensor_group);
            sysfs_remove_group(&client->dev.kobj, &X20XP_Led_group);
            sysfs_remove_group(&client->dev.kobj, &X20XP_FAN_group);
            sysfs_remove_group(&client->dev.kobj, &X20XP_PSU_group);
            break;
        case PCA9548_0X70:
        case PCA9548_0X71:
        case PCA9548_0X72:
        case PCA9548_0X73:
        case PCA9548_0X74:
        case PCA9548_0X75:
        case PCA9548_0X76:
            break;

        default:
            dev_dbg(&client->dev, "i2c_remove_CPLD failed (0x%x)\n", client->addr);
            break;
    }
  
    asterfusion_x20xp_cpld_remove_client(client);
    return 0;
}

static const struct i2c_device_id asterfusion_x20xp_cpld_id[] = {
    { "x20xp_cpld", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, asterfusion_x20xp_cpld_id);

static struct i2c_driver asterfusion_x20xp_cpld_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name = "x20xp_cpld",
    },
    .probe      = asterfusion_x20xp_cpld_probe,
    .remove     = asterfusion_x20xp_cpld_remove,
    .id_table   = asterfusion_x20xp_cpld_id,
    .address_list = normal_i2c,
};

static int __init asterfusion_x20xp_cpld_init(void)
{
    mutex_init(&list_lock);

    return i2c_add_driver(&asterfusion_x20xp_cpld_driver);
}

static void __exit asterfusion_x20xp_cpld_exit(void)
{
    i2c_del_driver(&asterfusion_x20xp_cpld_driver);
}

MODULE_AUTHOR("AF inc.");
MODULE_DESCRIPTION("x20xp_cpld driver");
MODULE_LICENSE("GPL");

module_init(asterfusion_x20xp_cpld_init);
module_exit(asterfusion_x20xp_cpld_exit);
