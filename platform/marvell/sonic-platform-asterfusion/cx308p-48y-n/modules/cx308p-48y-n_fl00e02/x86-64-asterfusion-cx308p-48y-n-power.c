/* An hwmon driver for Asterfusion CX308P-48Y-N Marvell i2c Module */
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#include "x86-64-asterfusion-cx308p-48y-n.h"
#include "x86-64-asterfusion-cx308p-48y-n-common.h"

ssize_t psu_status_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status[UART_MAX_SIZE] = "\0";
    u8 data_1[UART_MAX_SIZE] = {0};
    u8 data_2[UART_MAX_SIZE] = {0};
    int ret = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    ret = uart_read_cmd(UART_CMD_PSU, PSU_OP_STATUS, SUBCMD_DEFAULT, 3, status); //BMC 0x14 0xc0

    debug_print((KERN_DEBUG "DEBUG : PSU_PRESENT status = %x %x %x %x %x %x\n",status[1],status[2],status[3],status[4],status[5],status[6]));
    switch (attr->index)
    {
        case PSU_PRESENT:
            if (ret < 0)
            {
                sprintf(data_1, "PSU 1 is unknown\n");
                sprintf(data_2, "PSU 2 is unknown\n");
                break;
            }
			if (!(status[3] & 0xf))
			{
				sprintf(data_1, "PSU 1 is present\n");
			}
			else
			{
				sprintf(data_1, "PSU 1 is not present\n");
			}
			if (!(status[6] & 0xf))
			{
				sprintf(data_2, "PSU 2 is present\n");
			}
			else
			{
				sprintf(data_2, "PSU 2 is not present\n");
			}
            break;
        case PSU_STATUS:
            if (ret < 0)
            {
                sprintf(data_1, "PSU 1 is power unknown\n");
                sprintf(data_2, "PSU 2 is power unknown\n");
                break;
            }
            if (!status[1])
            {
                sprintf(data_1, "PSU 1 is not power Good\n");
            }
            else
            {
                sprintf(data_1, "PSU 1 is power Good\n");
            }
            if (!status[4])
            {
                sprintf(data_2, "PSU 2 is not power Good\n");
            }
            else
            {
                sprintf(data_2, "PSU 2 is power Good\n");
            }
            break;
    }
    return sprintf(buf, "%s%s", data_1, data_2);
}

ssize_t psu_module_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 module_num = 0;
    u8 psu_status[30] = {0};
	u8 integer = 0;
    u8 decimal = 0;
    int highbyte = 0;
    u8 lowbyte = 0;
    u8 type = 0;
    int j = 0;
    u8 data[UART_MAX_SIZE * 2] = {0};

    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    switch(attr->index)
    {
        case PSU_MODULE_1:
            module_num = 1;
            break;
        case PSU_MODULE_2:
            module_num = 2;
            break;
    }
	
	uart_read_cmd(UART_CMD_PSU, PSU_OP_INFO, SUBCMD_DEFAULT, 3, psu_status);

    integer = psu_status[1 + (module_num-1)*13];
    decimal = psu_status[2 + (module_num-1)*13];
    j += sprintf(data + j, "PSU %d VIN (V)  is %d.%d\n", module_num, integer, decimal);

    integer = psu_status[3 + (module_num-1)*13];
    decimal = psu_status[4 + (module_num-1)*13];
    j += sprintf(data + j, "PSU %d VOUT(V)  is %d.%d\n", module_num, integer, decimal);

    integer = psu_status[5 + (module_num-1)*13];
    decimal = psu_status[6 + (module_num-1)*13];
    j += sprintf(data + j, "PSU %d IIN (A)  is %d.%d\n", module_num, integer, decimal);

    integer = psu_status[7 + (module_num-1)*13];
    decimal = psu_status[8 + (module_num-1)*13];
    j += sprintf(data + j, "PSU %d IOUT(A)  is %d.%d\n", module_num, integer, decimal);

    highbyte = psu_status[11 + (module_num-1)*13];
    lowbyte = psu_status[12 + (module_num-1)*13];
    j += sprintf(data + j, "PSU %d PIN (W)  is %d\n", module_num, highbyte*256+lowbyte);

	highbyte = psu_status[9 + (module_num-1)*13];
    lowbyte = psu_status[10 + (module_num-1)*13];
    j += sprintf(data + j, "PSU %d POUT(W)  is %d\n", module_num, highbyte*256+lowbyte);

    type = psu_status[13 + (module_num-1)*13];
    if ( type == 0x01 ){
        j += sprintf(data + j, "PSU %d power input type is AC\n", module_num);
    } else if ( type == 0x02 ){
        j += sprintf(data + j, "PSU %d power input type is DC\n", module_num);
    } else {
        j += sprintf(data + j, "PSU %d power input type is unknown or unsupported\n", module_num);
    }

    return sprintf(buf, "%s", data);
}

ssize_t psu_direction_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 psu_status[UART_MAX_SIZE] = "\0";
    u8 data_1[UART_MAX_SIZE] = {0};
    u8 data_2[UART_MAX_SIZE] = {0};
    uart_read_cmd(UART_CMD_PSU, PSU_OP_STATUS, SUBCMD_DEFAULT, 3, psu_status);

    if (!(psu_status[3] & 0xf0))
    {
        sprintf(data_1, "PSU 1 is exhaust\n");
    }
    else
    {
        sprintf(data_1, "PSU 1 is intake\n");
    }
    if (!(psu_status[6] & 0xf0))
    {
        sprintf(data_2, "PSU 2 is exhaust\n");
    }
    else
    {
        sprintf(data_2, "PSU 2 is intake\n");
    }
    return sprintf(buf, "%s%s", data_1, data_2);
}

ssize_t psu_warning_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 psu_status[UART_MAX_SIZE] = "\0";
    u8 data_1[UART_MAX_SIZE] = {0};
    u8 data_2[UART_MAX_SIZE] = {0};
    uart_read_cmd(UART_CMD_PSU, PSU_OP_STATUS, SUBCMD_DEFAULT, 3, psu_status);

    if (!(psu_status[2] & 0xf))
    {
        sprintf(data_1, "PSU 1 is warning\n");
    }
    else
    {
        sprintf(data_1, "PSU 1 is not warning\n");
    }
    if (!(psu_status[5] & 0xf))
    {
        sprintf(data_2, "PSU 2 is warning\n");
    }
    else
    {
        sprintf(data_2, "PSU 2 is not warning\n");
    }
    return sprintf(buf, "%s%s", data_1, data_2);
}

ssize_t psu_direction_warning_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 psu_status[UART_MAX_SIZE] = "\0";
    u8 data_1[UART_MAX_SIZE] = {0};
    u8 data_2[UART_MAX_SIZE] = {0};
    uart_read_cmd(UART_CMD_PSU, PSU_OP_STATUS, SUBCMD_DEFAULT, 3, psu_status);

    if (!(psu_status[2] & 0xf0))
    {
        sprintf(data_1, "PSU 1 direction is not warning\n");
    }
    else
    {
        sprintf(data_1, "PSU 1 direction is warning\n");
    }
    if (!(psu_status[5] & 0xf0))
    {
        sprintf(data_2, "PSU 2 direction is not warning\n");
    }
    else
    {
        sprintf(data_2, "PSU 2 direction is warning\n");
    }
    return sprintf(buf, "%s%s", data_1, data_2);
}
