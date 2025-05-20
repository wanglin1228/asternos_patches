/*
 * asterfusion_x20xp_xxs_sfp_status.c - A driver to read and write the EEPROM on optical transceivers
 */


#undef Aster_DEBUG
/*#define Aster_DEBUG*/
#ifdef Aster_DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif /* DEBUG */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/types.h>

#include <linux/dmi.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>


#define NUM_ADDRESS                 2
/* The maximum length of a port name */
#define MAX_PORT_NAME_LEN           20
/* fundamental unit of addressing for EEPROM */
#define SFP_PAGE_SIZE               128

/*
 * Single address devices (eg QSFP) have 256 pages, plus the unpaged
 * low 128 bytes.  If the device does not support paging, it is
 * only 2 'pages' long.
 */
#define SFP_ARCH_PAGES                  256
#define ONE_ADDR_EEPROM_SIZE            ((1 + SFP_ARCH_PAGES) * SFP_PAGE_SIZE)
#define ONE_ADDR_EEPROM_UNPAGED_SIZE    (2 * SFP_PAGE_SIZE)

/*
 * Dual address devices (eg SFP) have 256 pages, plus the unpaged
 * low 128 bytes, plus 256 bytes at 0x50.  If the device does not
 * support paging, it is 4 'pages' long.
 */
#define TWO_ADDR_EEPROM_SIZE            ((3 + SFP_ARCH_PAGES) * SFP_PAGE_SIZE)
#define TWO_ADDR_EEPROM_UNPAGED_SIZE    (4 * SFP_PAGE_SIZE)
#define TWO_ADDR_NO_0X51_SIZE           (2 * SFP_PAGE_SIZE)

/*
 * flags to distinguish one-address (QSFP family) from two-address (SFP family)
 * If the family is not known, figure it out when the device is accessed
 */
#define ONE_ADDR                    1
#define TWO_ADDR                    2
#define CMIS_ADDR                   3

/* a few constants to find our way around the EEPROM */\
#define SFP_EEPROM_A0_ADDR          0x50
#define SFP_EEPROM_A2_ADDR          0x51
#define SFP_STATUS_ADDR             0x20
#define SFP_PAGE_SELECT_REG         0x7F
#define ONE_ADDR_PAGEABLE_REG       0x02
#define QSFP_NOT_PAGEABLE           (1<<2)
#define CMIS_NOT_PAGEABLE           (1<<7)
#define TWO_ADDR_PAGEABLE_REG       0x40
#define TWO_ADDR_PAGEABLE           (1<<4)
#define TWO_ADDR_0X51_REG           92
#define TWO_ADDR_0X51_SUPP          (1<<6)
#define SFP_ID_REG                  0
#define SFP_READ_OP                 0
#define SFP_WRITE_OP                1
#define SFP_EOF                     0  /* used for access beyond end of device */
#define CPLDA_SFP_NUM               8
#define CPLDB_SFP_NUM               8
#define CPLDC_SFP_NUM               8
#define CPLDD_SFP_NUM               8
#define CPLDE_SFP_NUM               8
#define CPLDF_SFP_NUM               8
#define CPLDG_SFP_NUM               4
#define MAX_PORT_NUM                30

#define PCA9548_0X70                0x70
#define PCA9548_0X71                0x71
#define PCA9548_0X72                0x72
#define PCA9548_0X73                0x73
#define PCA9548_0X74                0x74
#define PCA9548_0X75                0x75
#define PCA9548_0X76                0x76

#define SFP_SCL_BASE                0x0
#define SFP_PRESENT_BASE            0x1
#define SFP_RX_LOSS_BASE            0x1
#define SFP_TX_CTRL_BASE            0x6
#define QSFP_PRESENT_BASE           0x0

#define GET_BIT(data, bit, value)   value = (data >> bit) & 0x1
#define SET_BIT(data, bit)          data |= (1 << bit)
#define CLEAR_BIT(data, bit)        data &= ~(1 << bit)

static struct i2c_client* sfp_status_client;

extern int asterfusion_x20xp_cpld_read(unsigned short cpld_addr, u8 reg);
extern int asterfusion_x20xp_cpld_write(unsigned short cpld_addr, u8 reg, u8 value);
extern int asterfusion_x20xp_cpld_reset(void);
extern void asterfusion_x20xp_read_lock(void);
extern void asterfusion_x20xp_read_unlock(void);


#define GET_SFP_STATUS_SCL_ADDRESS(idx, reg, data) \
        reg = idx/8 + PCA9548_0X70;\
        data = (idx%8 < 4) ? 0x01 : 0x10

/* sudo i2cset -f -y 1 0x76 0x0 0x1 */
#define GET_QSFP_STATUS_SCL_ADDRESS(idx, reg, data) \
        reg = PCA9548_0X76;\
        data = 0x01

static ssize_t get_sfp_present(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    u8 reg = 0, data = 0, val = 0;

    asterfusion_x20xp_read_lock();
    DBG(printk(KERN_ALERT "%s - asterfusion_x20xp_read_lock for interface %d!\n", __func__, attr->index));

    asterfusion_x20xp_cpld_reset();
    GET_SFP_STATUS_SCL_ADDRESS(attr->index, reg, data);
    asterfusion_x20xp_cpld_write(reg, SFP_SCL_BASE, data);

    data = i2c_smbus_read_byte_data(client, SFP_PRESENT_BASE);
    // DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, SFP_PRESENT_BASE, data));
    GET_BIT(data, (attr->index % 4), val);

    asterfusion_x20xp_read_unlock();
    DBG(printk(KERN_ALERT "%s - asterfusion_x20xp_read_unlock for interface %d!\n", __func__, attr->index));

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_qsfp_present(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    u8 reg = 0, data = 0, val = 0;

    asterfusion_x20xp_read_lock();
    DBG(printk(KERN_ALERT "%s - asterfusion_x20xp_read_lock for interface %d!\n", __func__, attr->index));

    asterfusion_x20xp_cpld_reset();
    GET_QSFP_STATUS_SCL_ADDRESS(attr->index, reg, data);
    asterfusion_x20xp_cpld_write(reg, SFP_SCL_BASE, data);

    data = i2c_smbus_read_byte_data(client, QSFP_PRESENT_BASE);
    // DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, QSFP_PRESENT_BASE, data));
    GET_BIT(data, (attr->index % 6), val);

    asterfusion_x20xp_read_unlock();
    DBG(printk(KERN_ALERT "%s - asterfusion_x20xp_read_unlock for interface %d!\n", __func__, attr->index));

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_sfp_rx_loss(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    u8 reg = 0, data = 0, val = 0;

    asterfusion_x20xp_read_lock();

    asterfusion_x20xp_cpld_reset();
    GET_SFP_STATUS_SCL_ADDRESS(attr->index, reg, data);
    asterfusion_x20xp_cpld_write(reg, SFP_SCL_BASE, data);

    data = i2c_smbus_read_byte_data(client, SFP_RX_LOSS_BASE);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, SFP_RX_LOSS_BASE, data));
    GET_BIT(data, ((attr->index % 4) + 4), val);

    asterfusion_x20xp_read_unlock();

    return sprintf(buf, "%d\n", val);
}

static ssize_t get_sfp_tx_disable(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    u8 reg = 0, data = 0, val = 0;

    asterfusion_x20xp_read_lock();

    asterfusion_x20xp_cpld_reset();
    GET_SFP_STATUS_SCL_ADDRESS(attr->index, reg, data);
    asterfusion_x20xp_cpld_write(reg, SFP_SCL_BASE, data);

    data = i2c_smbus_read_byte_data(client, SFP_TX_CTRL_BASE);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, SFP_TX_CTRL_BASE, data));
    GET_BIT(data, (attr->index % 4), val);

    asterfusion_x20xp_read_unlock();

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

    asterfusion_x20xp_read_lock();

    asterfusion_x20xp_cpld_reset();
    GET_SFP_STATUS_SCL_ADDRESS(attr->index, reg, data);
    asterfusion_x20xp_cpld_write(reg, SFP_SCL_BASE, data);

    data = i2c_smbus_read_byte_data(client, SFP_TX_CTRL_BASE);
    DBG(printk(KERN_ALERT "%s - addr: 0x%x, reg: %x, data: %x\r\n", __func__, client->addr, SFP_TX_CTRL_BASE, data));

    if(val)
        SET_BIT(data, (attr->index % 4));
    else
        CLEAR_BIT(data, (attr->index % 4));

    i2c_smbus_write_byte_data(client, reg, data);

    asterfusion_x20xp_read_unlock();

    return count;
}


#define SFP_STATUS_ATTR(_num)  \
        static SENSOR_DEVICE_ATTR(sfp##_num##_present,  S_IRUGO,            get_sfp_present,    NULL,               _num-1);

SFP_STATUS_ATTR(1);SFP_STATUS_ATTR(2);SFP_STATUS_ATTR(3);SFP_STATUS_ATTR(4);SFP_STATUS_ATTR(5);SFP_STATUS_ATTR(6);SFP_STATUS_ATTR(7);SFP_STATUS_ATTR(8);SFP_STATUS_ATTR(9);
SFP_STATUS_ATTR(10);SFP_STATUS_ATTR(11);SFP_STATUS_ATTR(12);SFP_STATUS_ATTR(13);SFP_STATUS_ATTR(14);SFP_STATUS_ATTR(15);SFP_STATUS_ATTR(16);SFP_STATUS_ATTR(17);SFP_STATUS_ATTR(18);
SFP_STATUS_ATTR(19);SFP_STATUS_ATTR(20);SFP_STATUS_ATTR(21);SFP_STATUS_ATTR(22);SFP_STATUS_ATTR(23);SFP_STATUS_ATTR(24);SFP_STATUS_ATTR(25);SFP_STATUS_ATTR(26);SFP_STATUS_ATTR(27);
SFP_STATUS_ATTR(28);SFP_STATUS_ATTR(29);SFP_STATUS_ATTR(30);SFP_STATUS_ATTR(31);SFP_STATUS_ATTR(32);SFP_STATUS_ATTR(33);SFP_STATUS_ATTR(34);SFP_STATUS_ATTR(35);SFP_STATUS_ATTR(36);
SFP_STATUS_ATTR(37);SFP_STATUS_ATTR(38);SFP_STATUS_ATTR(39);SFP_STATUS_ATTR(40);SFP_STATUS_ATTR(41);SFP_STATUS_ATTR(42);SFP_STATUS_ATTR(43);SFP_STATUS_ATTR(44);SFP_STATUS_ATTR(45);
SFP_STATUS_ATTR(46);SFP_STATUS_ATTR(47);SFP_STATUS_ATTR(48);


#define SFP_TX_DISABLE_ATTR(_num)  \
        static SENSOR_DEVICE_ATTR(sfp##_num##_tx_ctrl,  S_IRUGO | S_IWUSR,  get_sfp_tx_disable, set_sfp_tx_disable, _num-1);  \
        static SENSOR_DEVICE_ATTR(sfp##_num##_rx_loss,  S_IRUGO,            get_sfp_rx_loss,    NULL,               _num-1);

SFP_TX_DISABLE_ATTR(1);SFP_TX_DISABLE_ATTR(2);SFP_TX_DISABLE_ATTR(3);SFP_TX_DISABLE_ATTR(4);SFP_TX_DISABLE_ATTR(5);SFP_TX_DISABLE_ATTR(6);SFP_TX_DISABLE_ATTR(7);SFP_TX_DISABLE_ATTR(8);SFP_TX_DISABLE_ATTR(9);
SFP_TX_DISABLE_ATTR(10);SFP_TX_DISABLE_ATTR(11);SFP_TX_DISABLE_ATTR(12);SFP_TX_DISABLE_ATTR(13);SFP_TX_DISABLE_ATTR(14);SFP_TX_DISABLE_ATTR(15);SFP_TX_DISABLE_ATTR(16);SFP_TX_DISABLE_ATTR(17);SFP_TX_DISABLE_ATTR(18);
SFP_TX_DISABLE_ATTR(19);SFP_TX_DISABLE_ATTR(20);SFP_TX_DISABLE_ATTR(21);SFP_TX_DISABLE_ATTR(22);SFP_TX_DISABLE_ATTR(23);SFP_TX_DISABLE_ATTR(24);SFP_TX_DISABLE_ATTR(25);SFP_TX_DISABLE_ATTR(26);SFP_TX_DISABLE_ATTR(27);
SFP_TX_DISABLE_ATTR(28);SFP_TX_DISABLE_ATTR(29);SFP_TX_DISABLE_ATTR(30);SFP_TX_DISABLE_ATTR(31);SFP_TX_DISABLE_ATTR(32);SFP_TX_DISABLE_ATTR(33);SFP_TX_DISABLE_ATTR(34);SFP_TX_DISABLE_ATTR(35);SFP_TX_DISABLE_ATTR(36);
SFP_TX_DISABLE_ATTR(37);SFP_TX_DISABLE_ATTR(38);SFP_TX_DISABLE_ATTR(39);SFP_TX_DISABLE_ATTR(40);SFP_TX_DISABLE_ATTR(41);SFP_TX_DISABLE_ATTR(42);SFP_TX_DISABLE_ATTR(43);SFP_TX_DISABLE_ATTR(44);SFP_TX_DISABLE_ATTR(45);
SFP_TX_DISABLE_ATTR(46);SFP_TX_DISABLE_ATTR(47);SFP_TX_DISABLE_ATTR(48);

#define QSFP_STATUS_ATTR(_num)  \
        static SENSOR_DEVICE_ATTR(qsfp##_num##_present, S_IRUGO,            get_qsfp_present,    NULL,               _num-1);

QSFP_STATUS_ATTR(49);QSFP_STATUS_ATTR(50);QSFP_STATUS_ATTR(51);QSFP_STATUS_ATTR(52);QSFP_STATUS_ATTR(53);QSFP_STATUS_ATTR(54);


static struct attribute *x20xp_xxs_sfp_status_attributes[] = {
    &sensor_dev_attr_sfp1_present.dev_attr.attr,
    &sensor_dev_attr_sfp2_present.dev_attr.attr,
    &sensor_dev_attr_sfp3_present.dev_attr.attr,
    &sensor_dev_attr_sfp4_present.dev_attr.attr,
    &sensor_dev_attr_sfp5_present.dev_attr.attr,
    &sensor_dev_attr_sfp6_present.dev_attr.attr,
    &sensor_dev_attr_sfp7_present.dev_attr.attr,
    &sensor_dev_attr_sfp8_present.dev_attr.attr,
    &sensor_dev_attr_sfp9_present.dev_attr.attr,
    &sensor_dev_attr_sfp10_present.dev_attr.attr,
    &sensor_dev_attr_sfp11_present.dev_attr.attr,
    &sensor_dev_attr_sfp12_present.dev_attr.attr,
    &sensor_dev_attr_sfp13_present.dev_attr.attr,
    &sensor_dev_attr_sfp14_present.dev_attr.attr,
    &sensor_dev_attr_sfp15_present.dev_attr.attr,
    &sensor_dev_attr_sfp16_present.dev_attr.attr,
    &sensor_dev_attr_sfp17_present.dev_attr.attr,
    &sensor_dev_attr_sfp18_present.dev_attr.attr,
    &sensor_dev_attr_sfp19_present.dev_attr.attr,
    &sensor_dev_attr_sfp20_present.dev_attr.attr,
    &sensor_dev_attr_sfp21_present.dev_attr.attr,
    &sensor_dev_attr_sfp22_present.dev_attr.attr,
    &sensor_dev_attr_sfp23_present.dev_attr.attr,
    &sensor_dev_attr_sfp24_present.dev_attr.attr,
    &sensor_dev_attr_sfp25_present.dev_attr.attr,
    &sensor_dev_attr_sfp26_present.dev_attr.attr,
    &sensor_dev_attr_sfp27_present.dev_attr.attr,
    &sensor_dev_attr_sfp28_present.dev_attr.attr,
    &sensor_dev_attr_sfp29_present.dev_attr.attr,
    &sensor_dev_attr_sfp30_present.dev_attr.attr,
    &sensor_dev_attr_sfp31_present.dev_attr.attr,
    &sensor_dev_attr_sfp32_present.dev_attr.attr,
    &sensor_dev_attr_sfp33_present.dev_attr.attr,
    &sensor_dev_attr_sfp34_present.dev_attr.attr,
    &sensor_dev_attr_sfp35_present.dev_attr.attr,
    &sensor_dev_attr_sfp36_present.dev_attr.attr,
    &sensor_dev_attr_sfp37_present.dev_attr.attr,
    &sensor_dev_attr_sfp38_present.dev_attr.attr,
    &sensor_dev_attr_sfp39_present.dev_attr.attr,
    &sensor_dev_attr_sfp40_present.dev_attr.attr,
    &sensor_dev_attr_sfp41_present.dev_attr.attr,
    &sensor_dev_attr_sfp42_present.dev_attr.attr,
    &sensor_dev_attr_sfp43_present.dev_attr.attr,
    &sensor_dev_attr_sfp44_present.dev_attr.attr,
    &sensor_dev_attr_sfp45_present.dev_attr.attr,
    &sensor_dev_attr_sfp46_present.dev_attr.attr,
    &sensor_dev_attr_sfp47_present.dev_attr.attr,
    &sensor_dev_attr_sfp48_present.dev_attr.attr,

    &sensor_dev_attr_qsfp49_present.dev_attr.attr,
    &sensor_dev_attr_qsfp50_present.dev_attr.attr,
    &sensor_dev_attr_qsfp51_present.dev_attr.attr,
    &sensor_dev_attr_qsfp52_present.dev_attr.attr,
    &sensor_dev_attr_qsfp53_present.dev_attr.attr,
    &sensor_dev_attr_qsfp54_present.dev_attr.attr,

    &sensor_dev_attr_sfp1_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp2_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp3_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp4_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp5_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp6_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp7_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp8_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp9_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp10_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp11_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp12_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp13_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp14_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp15_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp16_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp17_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp18_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp19_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp20_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp21_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp22_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp23_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp24_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp25_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp26_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp27_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp28_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp29_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp30_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp31_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp32_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp33_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp34_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp35_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp36_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp37_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp38_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp39_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp40_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp41_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp42_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp43_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp44_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp45_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp46_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp47_tx_ctrl.dev_attr.attr,
    &sensor_dev_attr_sfp48_tx_ctrl.dev_attr.attr,

    &sensor_dev_attr_sfp1_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp2_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp3_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp4_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp5_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp6_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp7_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp8_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp9_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp10_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp11_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp12_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp13_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp14_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp15_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp16_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp17_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp18_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp19_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp20_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp21_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp22_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp23_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp24_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp25_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp26_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp27_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp28_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp29_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp30_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp31_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp32_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp33_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp34_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp35_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp36_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp37_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp38_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp39_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp40_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp41_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp42_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp43_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp44_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp45_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp46_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp47_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp48_rx_loss.dev_attr.attr,

    NULL
};

static const struct attribute_group x20xp_xxs_sfp_status_group = { .attrs = x20xp_xxs_sfp_status_attributes};

static int x20xp_xxs_sfp_status_device_probe(struct i2c_client *client, const struct i2c_device_id *dev_id)
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
        case SFP_STATUS_ADDR:
            status = sysfs_create_group(&client->dev.kobj, &x20xp_xxs_sfp_status_group);
            break;

        default:
            dev_dbg(&client->dev, "i2c_check_sfp_status failed (0x%x)\n", client->addr);
            status = -EIO;
            goto exit;
            break;
    }

    if (status) {
        goto exit;
    }

    dev_info(&client->dev, "chip found\n");

    sfp_status_client = kzalloc(sizeof(struct i2c_client), GFP_KERNEL);
    
    if (!sfp_status_client) {
        dev_dbg(&client->dev, "Can't allocate sfp_status_client_node (0x%x)\n", client->addr);
        goto exit;
    }

    return 0;

exit:
    return status;
}

static int x20xp_xxs_sfp_status_device_remove(struct i2c_client *client)
{
    switch(client->addr)
    {
        case SFP_STATUS_ADDR:
            sysfs_remove_group(&client->dev.kobj, &x20xp_xxs_sfp_status_group);
            break;

        default:
            dev_dbg(&client->dev, "i2c_remove_sfp_status failed (0x%x)\n", client->addr);
            break;
    }

    kfree(sfp_status_client);
    return 0;
}

static const struct i2c_device_id x20xp_xxs_sfp_status_id[] = {
    { "x20xp_sfp_status", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, x20xp_xxs_sfp_status_id);

static struct i2c_driver x20xp_xxs_sfp_status_driver = {
    .driver = {
        .name     = "x20xp_sfp_status",
        .owner = THIS_MODULE,
    },
    .probe        = x20xp_xxs_sfp_status_device_probe,
    .remove       = x20xp_xxs_sfp_status_device_remove,
    .id_table     = x20xp_xxs_sfp_status_id,
};

static int __init x20xp_xxs_sfp_status_init(void)
{
    return i2c_add_driver(&x20xp_xxs_sfp_status_driver);
}

static void __exit x20xp_xxs_sfp_status_exit(void)
{
    i2c_del_driver(&x20xp_xxs_sfp_status_driver);
}

MODULE_AUTHOR(" AF inc. ");
MODULE_DESCRIPTION("x20xp_sfp_status driver");
MODULE_LICENSE("GPL");

module_init(x20xp_xxs_sfp_status_init);
module_exit(x20xp_xxs_sfp_status_exit);

