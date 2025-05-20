/* A hwmon driver for Asterfusion CX308p-48Y-N Marvell i2c Module */
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#include "x86-64-asterfusion-cx308p-48y-n.h"

extern struct i2c_client *CX_308P_i2c_client; //0x30 for COMe-to-CPLD1

ssize_t sfp_status_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status = -EPERM;
    u8 present_reg[7] = {0, 0x13, 0x12, 0x0f, 0x0e, 0x0b, 0x0a}; //CPLD1 - SFP present registers
    u8 rx_loss_reg[7] = {0, 0x11, 0x10, 0x0d, 0x0c, 0x09, 0x08}; //CPLD1 - SFP tx loss registers
    u8 tx_ctrl_reg[7] = {0, 0x1a, 0x19, 0x18, 0x17, 0x16, 0x15}; //CPLD1 - SFP tx disable registers
    u8 data[UART_MAX_SIZE * 10] = {0};
    u8 mask = 0x1;
    u8 port_num;
    u8 i,j;
    int k = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    if (attr->index == SFP_PRESENT)
    {
        port_num = 1;
        for (i = 1; i <= 6; i++)
        {
            status = i2c_smbus_read_byte_data(CX_308P_i2c_client, present_reg[i]);

            debug_print((KERN_DEBUG "DEBUG : SFP_PRESENT status = %x\n", status));
            for (j = 1; j <= 8; j++)
            {
                if (status & mask)
                {
                    k += sprintf(data + k, "SFP %02d is not present\n", port_num);
                }
                else
                {
                    k += sprintf(data + k, "SFP %02d is present\n", port_num);
                }
                port_num++;
                mask = mask << 1;
            }
            mask = 0x1;
        }
    }
    else if (attr->index == SFP_RX_LOSS)
    {
        port_num = 1;
        for (i = 1; i <= 6; i++)
        {
            status = i2c_smbus_read_byte_data(CX_308P_i2c_client, rx_loss_reg[i]);
            debug_print((KERN_DEBUG "DEBUG : SFP_PRESENT status = %x\n", status));
            for (j = 1; j <= 8; j++)
            {
                if (status & mask)
                {
                    k += sprintf(data + k, "SFP %02d loss of signal\n", port_num);
                }
                else
                {
                    k += sprintf(data + k, "SFP %02d signal detected\n", port_num);
                }
                port_num++;
                mask = mask << 1;
            }
            mask = 0x1;
        }
    }
    else if (attr->index == SFP_TX_STAT)
    {
        port_num = 1;
        for (i = 1; i <= 6; i++)
        {
            status = i2c_smbus_read_byte_data(CX_308P_i2c_client, tx_ctrl_reg[i]);
            debug_print((KERN_DEBUG "DEBUG : SFP_PRESENT status = %x\n",status));
            for (j = 1; j <= 8; j++)
            {
                if (status & mask)
                {
                    k += sprintf(data + k, "SFP %02d Disable TX\n", port_num);
                }
                else
                {
                    k += sprintf(data + k, "SFP %02d Enable TX\n", port_num);
                }
                port_num++;
                mask = mask << 1;
            }
            mask = 0x1;
        }
    }
    return sprintf(buf, "%s", data);
}

ssize_t sfp_tx_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u8 status = -EPERM;
    u8 result = -EPERM;
    u8 input = 0;
    u8 port_num = 0;
    u8 offset = 0;
    u8 reg = 0x0;
    u8 tx_ctrl_reg[7] = {0x00, 0x1a, 0x19, 0x18, 0x17, 0x16, 0x15}; //CPLD1 - SFP tx disable registers
    
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct Asterfusion_i2c_data *CPLD_1_data = i2c_get_clientdata(CX_308P_i2c_client);
    
    input = simple_strtol(buf, NULL, 10); //user input, 0 disable, 1 enable
    port_num = attr->index;
    if (port_num >= 1 && port_num <= 8)
    {
        reg = tx_ctrl_reg[1];
    }
    else if(port_num >= 9 && port_num <= 16)
    {
        reg = tx_ctrl_reg[2];
    }
    else if(port_num >= 17 && port_num <= 24)
    {
        reg = tx_ctrl_reg[3];
    }
    else if(port_num >= 25 && port_num <= 32)
    {
        reg = tx_ctrl_reg[4];
    }
    else if(port_num >= 33 && port_num <= 40)
    {
        reg = tx_ctrl_reg[5];
    }
    else if(port_num >= 41 && port_num <= 48)
    {
        reg = tx_ctrl_reg[6];
    }
    else 
    {
        printk(KERN_ALERT "sfp_tx_set wrong port index\n");
        return count;
    }
    
    offset = port_num % 8;
    if (offset == 0) {
        offset = 8;
    }
    status = i2c_smbus_read_byte_data(CX_308P_i2c_client, reg);
    debug_print((KERN_DEBUG "DEBUG : sfp_tx_set status = %x\n",status));
    // for hardware, 0 enable, 1 disable
    if(input == TURN_ON)
    {
        status &= ~(1 << (offset-1));
    }
    else if(input == TURN_OFF)
    {
        status |= (1 << (offset-1));
    }
    else
    {
        printk(KERN_ALERT "sfp_tx_set set wrong value\n");
        return count;
    }
    debug_print((KERN_DEBUG "DEBUG : sfp_tx_set value = %x\n",status));
    mutex_lock(&CPLD_1_data->update_lock);
    result = i2c_smbus_write_byte_data(CX_308P_i2c_client, reg, status);
    mutex_unlock(&CPLD_1_data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : sfp_tx_set result = %x\n",result));
    if (result < 0)
    {
        printk(KERN_ALERT "ERROR: sfp_tx_set sfp %d FAILED!\n", port_num);
    }
    else
    {
        debug_print((KERN_DEBUG "sfp_tx_set port %02d : %d\n", port_num, input));
    }
    return count;
}

ssize_t qsfp_reset_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u8 status = 0;
    u8 value  = 0;
    u16 i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Asterfusion_i2c_data *CPLD_1_data = i2c_get_clientdata(CX_308P_i2c_client);

    mutex_lock(&CPLD_1_data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : qsfp_reset_set mutex_lock\n"));
    if (attr->index == QSFP_RESET)
    {
        i = simple_strtol(buf, NULL, 10);   // user input, N for reset QSFP N (1 <= N <= 8)
        if (i >= 1 && i <= 8)
        {
            value  = 0;
            value  = i2c_smbus_read_byte_data(CX_308P_i2c_client, 0x14);
            debug_print((KERN_DEBUG "DEBUG : qsfp_reset_set value = %x\n",value));
            // for hardware, set bit N = 0 to reset QSFP N
            value &= ~(1 << (i - 1));
            debug_print((KERN_DEBUG "DEBUG : qsfp_reset_set set value = %x\n",value));
            status = i2c_smbus_write_byte_data(CX_308P_i2c_client, 0x14, value);
            if (status < 0)
            {
                printk(KERN_ALERT "ERROR: QSFP_RESET port %02d set FAILED!\n", i);
            }
            else
            {
                debug_print((KERN_DEBUG "QSFP %02d reset success\n", i));
                // reset the bit N to 1 after 100ms
                msleep(100);
                value |= (1 << (i - 1));
                debug_print((KERN_DEBUG "DEBUG : qsfp_reset_set reset value = %x\n",value));
                status = i2c_smbus_write_byte_data(CX_308P_i2c_client, 0x14, value);
                if (status < 0){
                    printk(KERN_ALERT "ERROR: QSFP_RESET port %02d reset FAILED!\n", i);
                }else{
                    debug_print((KERN_DEBUG "QSFP %02d reset reset success\n", i));
                }
            }
        }
        else
        {
            printk(KERN_ALERT "qsfp_reset_set wrong value\n");
        }
    }
    mutex_unlock(&CPLD_1_data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
    return count;
}

ssize_t qsfp_status_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status = -EPERM; //qsfp_status 01-08 port stat
    u8 mask = 0x1;
    u16 i;
    u8 data[UART_MAX_SIZE * 2] = {0};
    int k = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    if (attr->index == QSFP_PRESENT)
    {
        status = i2c_smbus_read_byte_data(CX_308P_i2c_client, 0x6);
        for (i = 1; i <= 8; i++)
        {
            if (status & mask)
            {
                k += sprintf(data + k, "QSFP %02d is not present\n", i);
            }
            else
            {
                k += sprintf(data + k, "QSFP %02d is present\n", i);
            }
            mask = mask << 1;
        }
        mask = 0x1;
    }
    if (attr->index == QSFP_INT)
    {
        status = i2c_smbus_read_byte_data(CX_308P_i2c_client, 0x07);
        debug_print((KERN_DEBUG "DEBUG : QSFP_INT status = %x\n",status));
        mask = 0x1;
        for (i = 1; i <= 4; i++)
        {
            if (status & mask)
            {
                k += sprintf(data + k, "QSFP %02d or %02d don't have interrupt signal\n", i*2-1, i*2);
            }
            else
            {
                k += sprintf(data + k, "QSFP %02d and %02d have interrupt signal\n", i*2-1, i*2);
            }
            mask = mask << 1;
        }
        mask = 0x1;
    }
    return sprintf(buf, "%s", data);
}


