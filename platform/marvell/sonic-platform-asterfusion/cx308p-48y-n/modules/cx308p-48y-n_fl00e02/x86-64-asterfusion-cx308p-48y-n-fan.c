/* An hwmon driver for Asterfusion CX308P-48Y-N Marvell i2c Module */
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#include "x86-64-asterfusion-cx308p-48y-n.h"
#include "x86-64-asterfusion-cx308p-48y-n-common.h"

ssize_t fan_status_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status[UART_MAX_SIZE] = {0};
    u8 data[UART_MAX_SIZE * 2] = {0};
    u16 fan_speed;
    int i, j = 0;
    int ret = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    sprintf(buf, "");
    if (attr->index == FAN_STATUS) 
    {
        for(i=1; i<=4; i++)
        {
			ret = uart_read_cmd(UART_CMD_FAN_STATUS, FAN_STATUS_1 + i - 1, FAN_GET_STATUS, 3, status);
			debug_print((KERN_DEBUG "DEBUG : FAN_STATUS %d status = %x\n", i, status));
            if (ret < 0)
            {
                j += sprintf(data + j, "Fan %d is unknown\n", i);
            }
            else if(!(status[1] & 0x2))
            {
                j += sprintf(data + j, "Fan %d is Good\n", i);
            }
            else
            {
                j += sprintf(data + j, "Fan %d is Fail\n", i);
            } 
        }
    }
    else if (attr->index == FAN_PRESENT) 
    {
        for(i=1; i<=4; i++)
        {
			ret = uart_read_cmd(UART_CMD_FAN_STATUS, FAN_STATUS_1 + i - 1, FAN_GET_STATUS, 3, status);
			debug_print((KERN_DEBUG "DEBUG : FAN_PRESENT %d status = %x\n", i, status));
            if(ret < 0)
            {
                j += sprintf(data + j, "Fan %d is unknown\n", i);
            }
            else if(status[1] & 0x1)
            {
                j += sprintf(data + j, "Fan %d is present\n", i);
            }
            else
            {
                j += sprintf(data + j, "Fan %d is not present\n", i);
            } 
        }
    }
    else if (attr->index == FAN_AIRFLOW) 
    {
        for(i=1; i<=4; i++)
        {
			ret = uart_read_cmd(UART_CMD_FAN_STATUS, FAN_STATUS_1 + i - 1, FAN_GET_STATUS, 3, status);
			debug_print((KERN_DEBUG "DEBUG : FAN_AIRFLOW %d status = %x\n", i, status));
            if(ret < 0)
            {
                j += sprintf(data + j, "Fan %d airflow is unknown\n", i);
            }
            else if(status[2] == 0x01)
            {
                j += sprintf(data + j, "Fan %d airflow is exhaust\n", i);
            }
            else
            {
                j += sprintf(data + j, "Fan %d airflow is intake\n", i);
            } 
        }
    }
    else if (attr->index == FAN_SPEED_RPM)
    {
		ret = uart_read_cmd(UART_CMD_FAN_INFO, SUBCMD_DEFAULT, SUBCMD_DEFAULT, 3, status);
        // first fan of couple
        for(i = 0; i < SYSFAN_MAX_NUM; i++)
        {
            // skip the fan which is not present
            if(ret < 0 || !(status[1] & (0x01<<i)))
            {
                j += sprintf(data + j, "FanModule%i Front : N/A RPM\n", i+1);
                j += sprintf(data + j, "FanModule%i Rear  : N/A RPM\n", i+1);
                continue;
            }

            // front fan of couple
            fan_speed = status[3 + i*4];
            fan_speed = (fan_speed<<8) + status[4 + i*4];
            j += sprintf(data + j, "FanModule%i Front : %d RPM\n", i+1, fan_speed);

            // rear fan of couple
            fan_speed = status[5 + i*4];
            fan_speed = (fan_speed<<8) + status[6 + i*4];
            j += sprintf(data + j, "FanModule%i Rear  : %d RPM\n", i+1, fan_speed);
        }
       
    }
    
    return sprintf(buf, "%s", data);
}

