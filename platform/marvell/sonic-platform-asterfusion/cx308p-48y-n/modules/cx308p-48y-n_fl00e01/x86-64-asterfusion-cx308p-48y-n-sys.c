/* An hwmon driver for Asterfusion CX308P-48Y-N Marvell i2c Module */
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#include "x86-64-asterfusion-cx308p-48y-n.h"
#include "x86-64-asterfusion-cx308p-48y-n-common.h"

extern struct i2c_client *CX_308P_i2c_client;
#define CPLD1_VERSION    0
#define CPLD2_VERSION    0x1c
#define CLK_SEL          0x3

ssize_t bmc_version_get(struct device *dev, struct device_attribute *attr, char *buf)
{
    u8 bmc_version_read_data[8] = {0};
    uart_read_cmd(UART_CMD_BMC_VERSION, SUBCMD_DEFAULT, SUBCMD_DEFAULT, 3, bmc_version_read_data);
    return sprintf(buf, "V%d.%dR%02d\n", bmc_version_read_data[1], bmc_version_read_data[2], bmc_version_read_data[3]);
}

ssize_t cpld_version_get(struct device *dev, struct device_attribute *attr, char *buf)
{
    u8 version_1   = -EPERM;
    u8 version_2   = -EPERM;
    version_1 = i2c_smbus_read_byte_data(CX_308P_i2c_client, CPLD1_VERSION);
    version_2 = i2c_smbus_read_byte_data(CX_308P_i2c_client, CPLD2_VERSION);
    return sprintf(buf, "CPLD1: %d\nCPLD2: %d\n", (version_1 & 0xff), (version_2 & 0xff));
}

ssize_t clk_sel_get(struct device *dev, struct device_attribute *attr, char *buf)
{
    u8 clk_byte = -EPERM;
    clk_byte = i2c_smbus_read_byte_data(CX_308P_i2c_client, CLK_SEL);
    return sprintf(buf, "clk_sel: %d (0: LOCAL_CLK; 1: RCVR0_CLK_OUT; 2: RCVR1_CLK_OUT)\n", (clk_byte & 0x3));
}

ssize_t clk_sel_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u8 clk_byte = -EPERM;
    u8 clk_sel = -EPERM;
    clk_byte = i2c_smbus_read_byte_data(CX_308P_i2c_client, CLK_SEL);
    clk_sel = simple_strtol(buf, NULL, 10);
    clk_byte = (clk_byte & 0xfc) | (clk_sel & 0x3);
    i2c_smbus_write_byte_data(CX_308P_i2c_client, CLK_SEL, clk_byte);
    return count;
}