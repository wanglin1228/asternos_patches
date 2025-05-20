#pragma GCC diagnostic ignored "-Wformat-zero-length"
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/serial_reg.h>
#include <linux/dmaengine.h>
#include <linux/dma/hsu.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/dmi.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/string.h>

#include "x86-64-asterfusion-cx532p-n.h"
#include "x86-64-asterfusion-cx532p-n-common.h"

#define DRIVER_VERSION  "1.0"
#define UART_TTYS1_OFFSET   (0x2f8)   /* /dev/ttyS1 */
#define UART_TTYS1_SIRQ		(3)   /* /dev/ttyS1 */
#define CX532P_N_I2C_CPLD_1     (2)
#define CX532P_N_CPLD_1         (0x40)

static struct mutex aster_cx532p_uart_lock;
struct i2c_client *aster_CPLD_40_client; //0x40 for SYS CPLD

static const unsigned short normal_i2c[] = { 0x40, I2C_CLIENT_END };

static const unsigned short ccitt_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

extern s32 i2c_smbus_xfer(struct i2c_adapter *adapter, u16 addr, unsigned short flags,
            char read_write, u8 command, int protocol,
            union i2c_smbus_data *data);

static void aster_cx532p_uart_read_lock(void)
{
    mutex_lock(&aster_cx532p_uart_lock);
}

static void aster_cx532p_uart_read_unlock(void)
{
    mutex_unlock(&aster_cx532p_uart_lock);
}

char Hex2Str(u8 dat,int idx)
{
    char temp = 0;
    if(idx)
        temp = dat>>4;
    else
        temp = dat&0xf;
    if(temp <= 9)
        return temp + '0';
    else
        return temp - 10 + 'A';     //全部转化为大写
    return 0;
}

void UnzipStr(char* dest,u8* src,int destNum)
{
    int i = 0;
    for(i = 0;i < destNum;i++)
    {
        if(i%2)
            dest[i] = Hex2Str(src[i/2],0);
        else
            dest[i] = Hex2Str(src[i/2],1);
    };
    dest[i] = '\n';
}

static unsigned short CRC16(u8 *q, int len)
{
    unsigned short crc = 0;

    while (len-- > 0)
        crc = (crc << 8) ^ ccitt_table[((crc >> 8) ^ (*q++ & 0xff)) & 0xff];
    return crc;
}

static int cmp_read_crc(u8 *ptr_str, int len)
{
	u8 crc_read[2] = {ptr_str[len - 2], ptr_str[len - 1]}; 
	unsigned short read_crc_cal = CRC16(ptr_str, len - 2);
//	printf("crc1 = %x %x, crc2 = %x %x\n", crc_read[0], crc_read[1], (read_crc_cal&0xff00)>>8, (read_crc_cal&0xff));
	if(crc_read[0] == ((read_crc_cal&0xff00)>>8) && crc_read[1] == (read_crc_cal&0xff))
		return 0;
	else
		return -1;
}

int get_bmc_data(u8 command, u8 fst_command, u8 sec_command, u8 vnum_command, union i2c_smbus_data *cx532p_bmc_read_data)
{
    unsigned int n = 0, size = 0, r_data_num = 0, i = 0;
    int ret = 0;
	u8 status = 0;
	unsigned int loop_times = 0;
	unsigned int max_rw_delay = 800000, max_single_r_delay = 20000;
	u8 temp[BUFFERSIZE]={0};
	char uart_order[50] = {0};
	char crc[10] = "\0";
	unsigned short crc_value = 0;
	strncpy(uart_order, "uart_", 5);
	if(vnum_command == 1)
		sprintf(&uart_order[5], "0x%02x", command);
	else if(vnum_command == 2)
		sprintf(&uart_order[5], "0x%02x_0x%02x", command, fst_command);
	else if(vnum_command == 3)
		sprintf(&uart_order[5], "0x%02x_0x%02x_0x%02x", command, fst_command, sec_command);

	crc_value = CRC16(uart_order, strlen(uart_order));
	sprintf(crc, "_%x%x#",(crc_value & 0xff00)>>8, crc_value & 0xff); 
	strcat(uart_order, crc);

	aster_cx532p_uart_read_lock();
	size = strlen(uart_order);
	do{
		while(!(inb(UART_TTYS1_OFFSET + UART_LSR) & UART_LSR_TEMT))
		{
			usleep_range(50,100);
		}
		outb(uart_order[n] & 0xff,UART_TTYS1_OFFSET);
		n++;
	}while(size != n);
		
	while(loop_times < max_rw_delay) {
		status = inb(UART_TTYS1_OFFSET + UART_LSR);
		if (status & UART_LSR_DR)
		{
			break;
		}
		loop_times++;
	}
	if(loop_times == max_rw_delay)
    {
        printk("uart timeout\n");
        ret=-ETIMEDOUT;
        goto _exit;
    }
	while(1){
		loop_times = 0;
		while(loop_times < max_single_r_delay) 
		{
			status = inb(UART_TTYS1_OFFSET + UART_LSR);
			if (status & UART_LSR_DR)
			{
				break;
			}
			loop_times++;
		}
		if(loop_times == max_single_r_delay)
		{
			break;
		}
		temp[r_data_num] = inb(UART_TTYS1_OFFSET);
		r_data_num++;
		
		if (r_data_num == BUFFERSIZE - 1)
		{
            printk("uart r_data_num oversize\n");
            ret=-EMSGSIZE;
            break;
		}
	}
	
	if(r_data_num > 2){
		if(!cmp_read_crc(temp, r_data_num)){
			for(i = 0; i < r_data_num - 2; i++)
				cx532p_bmc_read_data->block[i] = temp[i];
		}
        else
        {
            printk("uart crc wrong\n");
            ret = -EINVAL;
        }
	}

_exit:
	aster_cx532p_uart_read_unlock();
	return ret;
}
#if 0
static void serial_device_init(void)
{
	writeb(0, serial_membase + UART_IER);        /* Turn off interrupts - Port1 */ 
 	/* Communication Settings */
	writeb(UART_LCR_CONF_MODE_A, serial_membase + UART_LCR);  /* SET DLAB ON */
	/* Set Baud rate - Divisor Latch Low Byte */
	/*         0x01 = 115,200 BPS */
	/*         0x02 =  57,600 BPS */
	/*         0x06 =  19,200 BPS */
	/*         0x0C =   9,600 BPS */
	/*         0x18 =   4,800 BPS */
	/*         0x30 =   2,400 BPS */
	writeb(0x0c, serial_membase + UART_DLL);
	/* Set Baud rate - Divisor Latch High Byte */
	writeb(0x0, serial_membase + UART_DLM);  
    /* SET DLAB ON */
    writeb(UART_FCR_ENABLE_FIFO|UART_FCR7_64BYTE, serial_membase + UART_FCR);
	writeb(UART_LCR_CONF_MODE_B, serial_membase + UART_LCR);  
	writeb(0x0, serial_membase + UART_EFR);
	/* SET DLAB OFF */
	writeb(0x0, serial_membase + UART_LCR);
	
	writeb(UART_FCR_ENABLE_FIFO, serial_membase + UART_FCR);
	writeb(UART_LCR_WLEN8, serial_membase + UART_LCR);  /* 8 Bits, No Parity, 1 Stop Bit */
	/* FIFO Control Register */
//	writeb(UART_FCR_ENABLE_FIFO|UART_FCR_DMA_SELECT|UART_FCR_CLEAR_RCVR|UART_FCR_CLEAR_XMIT, serial_membase + UART_FCR); 
	/* Turn on DTR, RTS, and OUT2 */
	writeb(UART_MCR_OUT1 | UART_MCR_OUT2 | UART_MCR_RTS | UART_MCR_DTR, serial_membase + UART_MCR);  
    /* Interrupt when data received */
	writeb(0xf, serial_membase + UART_IER);  
	return ;
}
#endif
static int aster_cx532p_fl00e03_probe(struct i2c_client *client, const struct i2c_device_id *dev_id)
{
    struct aster_i2c_data *data;
    struct aster_i2c_data *aster_CPLD_40_data;
    int status;

	printk("<1>Pepp: CX532P probe Initiating...\n");
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA))
    {
        status = -EIO;
        goto exit;
    }

    data = kzalloc(sizeof(struct aster_i2c_data), GFP_KERNEL);
    if (!data)
    {
        printk(KERN_ALERT "kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }

    aster_CPLD_40_data = kzalloc(sizeof(struct aster_i2c_data), GFP_KERNEL);
    if (!aster_CPLD_40_data)
    {
        printk(KERN_ALERT "kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }
    
    i2c_set_clientdata(client, data);
    i2c_set_clientdata(aster_CPLD_40_client, aster_CPLD_40_data);

    mutex_init(&aster_cx532p_uart_lock);
    mutex_init(&aster_CPLD_40_data->update_lock);

    aster_CPLD_40_data->valid = 0;
    mutex_init(&aster_CPLD_40_data->update_lock);
    dev_info(&client->dev, "chip found\n");
    
    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &sys_info_group);
    if (status)
    {
        goto exit_free;
    }
    
    status = sysfs_create_group(&client->dev.kobj, &cx532p_core_voltage_group);
    if (status)
    {
        goto exit_free;
    }
    
    status = sysfs_create_group(&client->dev.kobj, &cx532p_eeprom_group);
    if (status)
    {
        goto exit_free;
    }

    status = sysfs_create_group(&client->dev.kobj, &cx532p_fan_group);
    if (status)
    {
        goto exit_free;
    }
    
    status = sysfs_create_group(&client->dev.kobj, &cx532p_lm_sensor_group);
    if (status)
    {
        goto exit_free;
    }
    
    status = sysfs_create_group(&client->dev.kobj, &cx532p_psu_group);
    if (status)
    {
        goto exit_free;
    }
    
    status = sysfs_create_group(&client->dev.kobj, &cx532p_qsfp_group);
    if (status)
    {
        goto exit_free;
    }

    aster_CPLD_40_data->hwmon_dev = hwmon_device_register(&client->dev);
    if (IS_ERR(aster_CPLD_40_data->hwmon_dev))
    {
        status = PTR_ERR(aster_CPLD_40_data->hwmon_dev);
        goto exit_remove;
    }
    dev_info(&client->dev, "%s: '%s'\n", dev_name(aster_CPLD_40_data->hwmon_dev), client->name);
    return 0;

exit_remove:
    sysfs_remove_group(&client->dev.kobj, &sys_info_group);
    sysfs_remove_group(&client->dev.kobj, &cx532p_core_voltage_group);
    sysfs_remove_group(&client->dev.kobj, &cx532p_eeprom_group);
    sysfs_remove_group(&client->dev.kobj, &cx532p_fan_group);
    sysfs_remove_group(&client->dev.kobj, &cx532p_lm_sensor_group);
    sysfs_remove_group(&client->dev.kobj, &cx532p_psu_group);
    sysfs_remove_group(&client->dev.kobj, &cx532p_qsfp_group);

exit_free:
    kfree(data);
    kfree(aster_CPLD_40_data);
exit:
    return status;
}

static int aster_cx532p_fl00e03_remove(struct i2c_client *client)
{
    struct aster_i2c_data *data = i2c_get_clientdata(client);
    printk("remove cx532p client ok\n");
    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &sys_info_group);
    sysfs_remove_group(&client->dev.kobj, &cx532p_core_voltage_group);
    sysfs_remove_group(&client->dev.kobj, &cx532p_eeprom_group);
    sysfs_remove_group(&client->dev.kobj, &cx532p_fan_group);
    sysfs_remove_group(&client->dev.kobj, &cx532p_lm_sensor_group);
    sysfs_remove_group(&client->dev.kobj, &cx532p_psu_group);
    sysfs_remove_group(&client->dev.kobj, &cx532p_qsfp_group);
    kfree(data);
    return 0;
}

static const struct i2c_device_id aster_cx532p_fl00e03_i2c_id[] =
{
    { "FL00E03_CPLD_40", 0 },
    {},
};
MODULE_DEVICE_TABLE(i2c, aster_cx532p_fl00e03_i2c_id);

static struct i2c_driver aster_cx532p_fl00e03_driver =
{
    .class        = I2C_CLASS_HWMON,
    .driver =
    {
        .name     = "CX532P_N_FL00E03",
    },
    .probe        = aster_cx532p_fl00e03_probe,
    .remove       = aster_cx532p_fl00e03_remove,
    .id_table     = aster_cx532p_fl00e03_i2c_id,
    .address_list = normal_i2c,
};

/*For main Switch board*/
static struct i2c_board_info aster_cx532p_fl00e03_CPLD_40_info[] __initdata =
{
    {
        I2C_BOARD_INFO("FL00E03_CPLD_40", CX532P_N_CPLD_1),
        .platform_data = NULL,
    },
};

static int __init aster_cx532p_fl00e03_init(void)
{
    int ret;
    struct i2c_adapter *i2c_adap_40;

    i2c_adap_40 = i2c_get_adapter(CX532P_N_I2C_CPLD_1);
    printk("SMBus I801\n");

    if (i2c_adap_40 == NULL)
    {
        printk("ERROR: i2c_get_adapter 2 FAILED!\n");
        return -1;
    }
    aster_CPLD_40_client = i2c_new_client_device(i2c_adap_40, &aster_cx532p_fl00e03_CPLD_40_info[0]);

    if (aster_CPLD_40_client == NULL)
    {
        printk("ERROR: i2c_new_device FAILED!\n");
        return -1;
    }
    
    i2c_put_adapter(i2c_adap_40);
    ret = i2c_add_driver(&aster_cx532p_fl00e03_driver);
    printk(KERN_ALERT "CX532P-N i2c Driver Version: %s\n", DRIVER_VERSION);
    printk(KERN_ALERT "CX532P-N i2c Driver INSTALL SUCCESS(%d)\n", ret);
    return ret;
}

static void __exit aster_cx532p_fl00e03_exit(void)
{
    i2c_unregister_device(aster_CPLD_40_client);
    i2c_del_driver(&aster_cx532p_fl00e03_driver);
    printk(KERN_ALERT "CX532P-N Driver UNINSTALL SUCCESS\n");
}

module_init(aster_cx532p_fl00e03_init);
module_exit(aster_cx532p_fl00e03_exit);

MODULE_DESCRIPTION("aster CX532P-N Driver");
MODULE_AUTHOR("wuyawen");
MODULE_LICENSE("GPL");
