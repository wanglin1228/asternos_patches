/* An hwmon driver for Asterfusion CX308P-48Y-N Marvell i2c Module */
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#include <linux/io.h>
#include <linux/pci.h>
#include "x86-64-asterfusion-cx308p-48y-n.h"

static void __iomem *serial_membase = NULL;
static struct pci_dev * g_pdev = NULL;
static struct mutex aster_uart_lock;
struct i2c_client *CX_308P_i2c_client;
static u8 uart_ttys1_irq = 0;
/* Addresses scanned */
static const unsigned short normal_i2c[] = { 0x40, I2C_CLIENT_END };

int read_8bit_temp(u8 sign,u8 value)
{
    int result = 0;
    if(sign)
    {
        //printf("read_8bit_temp UP %d\n", value & 0x80);
        value = ~(value)+1;
        result = value;
        return result;
    }
    else
    {
        //printf("read_8bit_temp DOWN %d\n", value & 0x80);
        result = value;
        return result;
    }
}

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

static unsigned short CRC16(unsigned char *q, int len)
{
    unsigned short crc = 0;

    while (len-- > 0)
        crc = (crc << 8) ^ ccitt_table[((crc >> 8) ^ (*q++ & 0xff)) & 0xff];
    return crc;
}

static int check_read_crc(unsigned char *string, int len)
{
	unsigned char crc_read[2] = {string[len - 2], string[len - 1]}; 
	unsigned short read_crc_cal = CRC16(string, len - 2);

	if(crc_read[0] == ((read_crc_cal&0xff00)>>8) && crc_read[1] == (read_crc_cal&0xff))
		return 0;
	else
		return -1;
}


int uart_read_cmd(u8 cmd, u8 sub_cmd_1, u8 sub_cmd_2, u8 n_para, u8 *r_data)
{
	int i;
    int ret = 0;
	u32 loop_times;
	unsigned int max_rw_delay = 800000, max_single_r_delay = 20000;
	int status = 0, w_data_num = 0, r_data_num = 0, size = 0;
	char uart_order[50] = "\0";
	u8 temp[UART_MAX_SIZE]="\0";
	char crc[10] = "\0";
	unsigned short crc_value = 0;
	strncpy(uart_order, "uart_", 5);
	
	if(r_data == NULL)
		return -EINVAL;
	
	if(n_para == 1)
		sprintf(&uart_order[5], "0x%02x", cmd);
	else if(n_para == 2)
		sprintf(&uart_order[5], "0x%02x_0x%02x", cmd, sub_cmd_1);
	else if(n_para == 3)
		sprintf(&uart_order[5], "0x%02x_0x%02x_0x%02x", cmd, sub_cmd_1, sub_cmd_2);
	else
		return -EINVAL;
	
    mutex_lock(&aster_uart_lock);
	
	crc_value = CRC16(uart_order, strlen(uart_order));
	sprintf(crc, "_%x%x#",(crc_value & 0xff00)>>8, crc_value & 0xff); 
	strcat(uart_order, crc);
	
	size = strlen(uart_order);
	do{
		while(!(readb(serial_membase + UART_LSR) & UART_LSR_TEMT))
			usleep_range(50,100);
		writeb(uart_order[w_data_num] & 0xff, serial_membase);
		w_data_num++;
	}while(size != w_data_num);

	while(loop_times < max_rw_delay) {
		status = readb(serial_membase + UART_LSR);
		if (status & 1)
			break;
		loop_times++;
	}
	if(loop_times == max_rw_delay)
    {
        printk("uart timeout\n");
        ret=-ETIMEDOUT;
        goto _exit;
    }

    do {
        loop_times = 0;
        while(loop_times < max_single_r_delay) {
            status = readb(serial_membase + UART_LSR);
            if (status & 1) {
                temp[r_data_num] = readb(serial_membase);
                r_data_num++;
                break;
            }
            else
            {
                loop_times++;
            }
        }

        if(loop_times == max_single_r_delay)
        {
            break;
        }
        if (r_data_num == UART_MAX_SIZE - 1) {
            printk("uart r_data_num oversize\n");
            ret=-EMSGSIZE;
            goto _exit;
        }
    }while (status & 1);

	if(r_data_num > 2){
		if(!check_read_crc(temp, r_data_num)){
			for(i = 0; i < r_data_num - 2; i++)
			{	
                r_data[i] = temp[i];
            }
		}
        else
        {
            printk("uart crc wrong\n");
            ret = -EINVAL;
        }
	}

_exit:
    mutex_unlock(&aster_uart_lock);
	return ret;
}

static int aster_cx308p_uart_init(void)
{
    u8 times = 0;
    struct mid8250 *mid = NULL;
    struct hsu_dma_chip *chip = NULL;
    struct pci_dev *pdev = NULL;
	printk("<1>Pepp: Serial Module Initiating...\n");
    while (times != 0xff)
    {
        pdev = pci_get_device(PCI_VENDOR_ID_INTEL, PCI_SERIAL_DEVICE_ID, pdev);
        /* remember to judge whether pdev is NULL or not, otherwise may encounter bug */
        if (pdev != NULL && pdev->pin == UART_TTYS1_PIN)
        {
            g_pdev = pdev;
            uart_ttys1_irq = pdev->irq;
            break;
        }
        times++;
    }
    if (NULL != g_pdev && uart_ttys1_irq != 0)
    {
    	mid = (struct mid8250 *)pci_get_drvdata(pdev);
        chip = &mid->dma_chip;
       	printk("irq %d membase %p length %d", pdev->irq, chip->regs, chip->length);
    	serial_membase = chip->regs;
    	disable_irq(uart_ttys1_irq);
	//	serial_device_init();
    }
	return 0;
}

static void aster_cx308p_uart_exit(void)
{
	if (NULL != g_pdev)
	{
		pci_dev_put(g_pdev);
	}
    if (uart_ttys1_irq != 0)
    {
	    enable_irq(uart_ttys1_irq);
    }
	printk("<1>Pepp: Good-bye, kernel!\n");
}

static int Asterfusion_cx308p_e01_device_probe(struct i2c_client *client, const struct i2c_device_id *dev_id)
{
	struct Asterfusion_i2c_data *data;
    int status;
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA))
    {
        status = -EIO;
        goto exit;
    }
    data = kzalloc(sizeof(struct Asterfusion_i2c_data), GFP_KERNEL);
    if (!data)
    {
        printk(KERN_ALERT "kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);

    mutex_init(&aster_uart_lock);

	data->valid = 0;
    mutex_init(&data->update_lock);
    dev_info(&client->dev, "chip found\n");
    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &sys_info_group);
    if (status)
    {
        goto exit_free;
    }
    
    status = sysfs_create_group(&client->dev.kobj, &CX308P_PSU_group);
    if (status)
    {
        goto exit_free;
    }
    status = sysfs_create_group(&client->dev.kobj, &CX308P_INT_group);
    if (status)
    {
        goto exit_free;
    }
    status = sysfs_create_group(&client->dev.kobj, &CX308P_SFP_group);
    if (status)
    {
        goto exit_free;
    }
    status = sysfs_create_group(&client->dev.kobj, &CX308P_QSFP_group);
    if (status)
    {
        goto exit_free;
    }
    status = sysfs_create_group(&client->dev.kobj, &CX308P_FAN_group);
    if (status)
    {
        goto exit_free;
    }
    status = sysfs_create_group(&client->dev.kobj, &CX308P_THERMAL_group);
    if (status)
    {
        goto exit_free;
    }
    status = sysfs_create_group(&client->dev.kobj, &CX308P_EEPROM_group);
    if (status)
    {
        goto exit_free;
    }

    data->hwmon_dev = hwmon_device_register(&client->dev);
    if (IS_ERR(data->hwmon_dev))
    {
        status = PTR_ERR(data->hwmon_dev);
        goto exit_remove;
    }
    dev_info(&client->dev, "%s: '%s'\n", dev_name(data->hwmon_dev), client->name);
    return 0;
exit_remove:
    sysfs_remove_group(&client->dev.kobj, &sys_info_group);
    sysfs_remove_group(&client->dev.kobj, &CX308P_PSU_group);
    sysfs_remove_group(&client->dev.kobj, &CX308P_INT_group);
    sysfs_remove_group(&client->dev.kobj, &CX308P_SFP_group);
    sysfs_remove_group(&client->dev.kobj, &CX308P_QSFP_group);
    sysfs_remove_group(&client->dev.kobj, &CX308P_FAN_group);
    sysfs_remove_group(&client->dev.kobj, &CX308P_THERMAL_group);
	sysfs_remove_group(&client->dev.kobj, &CX308P_EEPROM_group);

exit_free:
    kfree(data);
exit:
    return status;
}

static int Asterfusion_cx308p_e01_device_remove(struct i2c_client *client)
{
    struct Asterfusion_i2c_data *data = i2c_get_clientdata(client);
    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &sys_info_group);
    sysfs_remove_group(&client->dev.kobj, &CX308P_PSU_group);
    sysfs_remove_group(&client->dev.kobj, &CX308P_INT_group);
    sysfs_remove_group(&client->dev.kobj, &CX308P_SFP_group);
    sysfs_remove_group(&client->dev.kobj, &CX308P_QSFP_group);
    sysfs_remove_group(&client->dev.kobj, &CX308P_FAN_group);
    sysfs_remove_group(&client->dev.kobj, &CX308P_THERMAL_group);
	sysfs_remove_group(&client->dev.kobj, &CX308P_EEPROM_group);

    kfree(data);
    return 0;
}

static const struct i2c_device_id Asterfusion_cx308p_e01_i2c_id[] =
{
    { "CX308P_e01_i2c", 3 },
    {},
};
MODULE_DEVICE_TABLE(i2c, Asterfusion_cx308p_e01_i2c_id);

static struct i2c_driver Asterfusion_cx308p_e01_i2c_driver =
{
    .class        = I2C_CLASS_HWMON,
    .driver =
    {
        .name     = "CX308P_e01_i2c",
    },
    .probe        = Asterfusion_cx308p_e01_device_probe,
    .remove       = Asterfusion_cx308p_e01_device_remove,
    .id_table     = Asterfusion_cx308p_e01_i2c_id,
    .address_list = normal_i2c,
};

/*For main Switch board*/
static struct i2c_board_info CX308P_e01_i2c_info[] __initdata =
{
    {
        I2C_BOARD_INFO("CX308P_e01_i2c", 0x40),
        .platform_data = NULL,
    },
};

static int __init Asterfusion_cx308p_e01_module_init(void)
{
    int ret;
    struct i2c_adapter *i2c_adap;
#if 0
    int cmp;
    char keyword[] = "SMBus I801";
    char buf1[128];
    struct file *fp;  
    mm_segment_t fs;  
    loff_t pos; 

    printk("Open file...\n");  
    fp = filp_open("/sys/class/i2c-dev/i2c-0/name", O_RDONLY , 0644);  
    if (IS_ERR(fp)) {  
        printk("Open file FAILED\n");  
        return -1;  
    } 

    fs = get_fs();  
    set_fs(KERNEL_DS);
    pos = 0;
    vfs_read(fp, buf1, sizeof(buf1), &pos);
    printk("Detect %s\n", buf1);
    cmp = strncmp(keyword, buf1, sizeof(keyword)-1);
    set_fs(fs);

    filp_close(fp, NULL);

    if(cmp == 0)
    {
        i2c_adap = i2c_get_adapter(0);
        printk("SMBus I801 is at bus 0\n");
    }
    else
    {
        i2c_adap = i2c_get_adapter(1);
        printk("SMBus I801 is at bus 1\n");
    }
#endif
    i2c_adap = i2c_get_adapter(2);

    debug_print((KERN_DEBUG "Asterfusion_i2c_init\n"));
    if (i2c_adap == NULL)
    {
        printk("ERROR: i2c_get_adapter FAILED!\n");
        return -1;
    }

    CX_308P_i2c_client = i2c_new_client_device(i2c_adap, &CX308P_e01_i2c_info[0]);
    if (CX_308P_i2c_client == NULL)
    {
        printk("ERROR: CX_308P_i2c_client FAILED!\n");
        return -1;
    }

    i2c_put_adapter(i2c_adap);
    aster_cx308p_uart_init();
    ret = i2c_add_driver(&Asterfusion_cx308p_e01_i2c_driver);
    printk(KERN_ALERT "CX308P-48Y-N i2c Driver Version: %s\n", DRIVER_VERSION);
    printk(KERN_ALERT "CX308P-48Y-N i2c Driver INSTALL SUCCESS\n");
    return ret;
}

static void __exit Asterfusion_cx308p_e01_module_exit(void)
{
    aster_cx308p_uart_exit();
	i2c_unregister_device(CX_308P_i2c_client);
    i2c_del_driver(&Asterfusion_cx308p_e01_i2c_driver);
    printk(KERN_ALERT "CX308P-48Y-N i2c Driver UNINSTALL SUCCESS\n");
}

MODULE_AUTHOR("Asterfusion Inc.");
MODULE_DESCRIPTION("Asterfusion CX308P-48Y-N i2c Driver");
MODULE_LICENSE("GPL");

/* To driver developer: 
 *     Main variable names of this driver has been added a unique prefix(e.g. "e03").
 *     This is to distinguish from the neighbor driver(s) and is absolutely necessary.
 *     Because if two (or more) drivers which have same name exist at the same time on a system,
 *     install any one of them will install all of them! And this might not what we want.
 *     So we have to add the prefix even it increases the difficulty of reading. :)
 */
module_init(Asterfusion_cx308p_e01_module_init);
module_exit(Asterfusion_cx308p_e01_module_exit);
