/* An hwmon driver for aster cx532p-n Innovium i2c Module */
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#include "x86-64-asterfusion-cx532p-n.h"
#include "x86-64-asterfusion-cx532p-n-qsfp.h"

#define ENABLE          1
#define DISABLE         0

/* i2c_client Declaration */
extern struct i2c_client *aster_CPLD_40_client; //0x40 for Port 0-33
/* end of i2c_client Declaration */

/* implement i2c_function */

ssize_t qsfp_reset_all_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int value   = 0x0;
    u8 result  = 0;
    int input   = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct aster_i2c_data *aster_CPLD_40_data = i2c_get_clientdata(aster_CPLD_40_client);
    
    mutex_lock(&aster_CPLD_40_data->update_lock);
    if (attr->index == QSFP_RESET_ALL)
    {
        input = simple_strtol(buf, NULL, 10);
        if (input == QSFP_RESET)
        {
            value = 0x00;
        }
        else
        {
            printk(KERN_ALERT "qsfp_reset_all_set wrong value\n");
            return count;
        }
        result += i2c_smbus_write_byte_data(aster_CPLD_40_client, QSFP_RESET_REG_0, value);
        result += i2c_smbus_write_byte_data(aster_CPLD_40_client, QSFP_RESET_REG_1, value);
        result += i2c_smbus_write_byte_data(aster_CPLD_40_client, QSFP_RESET_REG_2, value);
        result += i2c_smbus_write_byte_data(aster_CPLD_40_client, QSFP_RESET_REG_3, value);
        
        if(result != 0)
        {
            printk(KERN_ALERT "qsfp_reset_all_set FAILED\n");
            return count;
        }
    }
    mutex_unlock(&aster_CPLD_40_data->update_lock);
    return count;
}

ssize_t qsfp_reset_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int status = -EPERM;
    int result = 0;
    int input  = 0;
    int port_index = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct aster_i2c_data *aster_CPLD_40_data = i2c_get_clientdata(aster_CPLD_40_client);
    
    port_index = attr->index;
    input = simple_strtol(buf, NULL, 10);
    mutex_lock(&aster_CPLD_40_data->update_lock);

    
    status = i2c_smbus_read_byte_data(aster_CPLD_40_client, qsfp_reset_regs[port_index][0]);
    if( input == QSFP_RESET)
    {
        status |= qsfp_reset_regs[port_index][1];
        result = i2c_smbus_write_byte_data(aster_CPLD_40_client, qsfp_reset_regs[port_index][0], status);
        if (result < 0)
        {
            printk(KERN_ALERT "ERROR: qsfp_reset_set FAILED!\n");
        }
    }
    else
    {
        printk(KERN_ALERT "ERROR: qsfp_reset_set WRONG VALUE\n");
    }
    
    mutex_unlock(&aster_CPLD_40_data->update_lock);
    
    return count;
}

ssize_t qsfp_present_all_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status   = -EPERM;
    u64 result  = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 data_1[BUFFERSIZE] = {0};
    u8 data_2[BUFFERSIZE] = {0};
    
    if (attr->index == QSFP_PRESENT_ALL)
    {
        status = i2c_smbus_read_byte_data(aster_CPLD_40_client, QSFP_PRESENT_REG_AUX); 
        result = (~((status >> 2) & 0x2) | (status & 0x1)) & 0x3;
        sprintf(data_1, "0x%llx", result);

        status = i2c_smbus_read_byte_data(aster_CPLD_40_client, QSFP_PRESENT_REG_3);  //29-32
        result = status & 0xff;
        status = i2c_smbus_read_byte_data(aster_CPLD_40_client, QSFP_PRESENT_REG_2);  //25-28
        result = (result << 8) | (status & 0xff);
        status = i2c_smbus_read_byte_data(aster_CPLD_40_client, QSFP_PRESENT_REG_1); //21-24
        result = (result << 8) | (status & 0xff);
        status = i2c_smbus_read_byte_data(aster_CPLD_40_client, QSFP_PRESENT_REG_0); //17-20
        result = (result << 8) | (status & 0xff);
        result = (~(result)) & 0xffffffff;
        sprintf(data_2, "%08llx\n", result);
    }
    return sprintf(buf, "%s%s", data_1, data_2);
}

ssize_t qsfp_present_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    int port_index = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 data[BUFFERSIZE] = {0};
    
    port_index = attr->index;

    status = i2c_smbus_read_byte_data(aster_CPLD_40_client, qsfp_present_regs[port_index][0]);
    
    if (status & qsfp_present_regs[port_index][1])
    {
        sprintf(data, "%d\n", DISABLE);
    }
    else
    {
        sprintf(data, "%d\n", ENABLE);
    }
    
    return sprintf(buf, "%s", data);
}

ssize_t sfp_tx_disable_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct aster_i2c_data *aster_CPLD_40_data = i2c_get_clientdata(aster_CPLD_40_client);
    int port_index = attr->index;
    u8 value   = 0;
    u8 result  = 0;

    mutex_lock(&aster_CPLD_40_data->update_lock);
    if (port_index == 1)
        value = 0x4;
    else
        value = 0x20;
    result = i2c_smbus_read_byte_data(aster_CPLD_40_client, QSFP_TXDIS_REG_AUX);
    switch(result & value){
        case ENABLE:
            value = result & (~(value));
            break;
        case DISABLE:
            value = result | value;
            break;
        default:
            return count;
    }
    result = i2c_smbus_write_byte_data(aster_CPLD_40_client, QSFP_TXDIS_REG_AUX, value);

    mutex_unlock(&aster_CPLD_40_data->update_lock);
    
    return count;
}

ssize_t xport_led_mode_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct aster_i2c_data *aster_CPLD_40_data = i2c_get_clientdata(aster_CPLD_40_client);
    int input = simple_strtol(buf, NULL, 10);
    u8 value   = 0;
    u8 result  = 0;

    if (input != 0)
        value = 1;

    mutex_lock(&aster_CPLD_40_data->update_lock);
    result = i2c_smbus_write_byte_data(aster_CPLD_40_client, XPORT_LED_MODE_REG, value);
    mutex_unlock(&aster_CPLD_40_data->update_lock);

    printk(KERN_NOTICE "xport_led_mode_set %d,%d\n", value, result);

    return count;
}
