#ifndef _CX532P_PLATFORM_H
#define _CX532P_PLATFORM_H
#include <linux/dmaengine.h>
#include <linux/dma/hsu.h>
#include <linux/8250_pci.h>
#include <linux/serial_8250.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>

#define BUFFERSIZE	(128)
enum aster_uart_cmd_attributes{
	UART_CMD_EEPROM = 0x1,
	UART_CMD_TEMP = 0x4,
	UART_CMD_FAN_INFO = 0x5,
	UART_CMD_FAN_STATUS = 0x6,
	UART_CMD_FAN_OP = 0x7,
	UART_CMD_PAYLOAD_INFO = 0x8,
	UART_CMD_BMC_OP = 0x9,
	UART_CMD_PAYLOAD_OP,
	UART_CMD_PSU,
	UART_CMD_BMC_VERSION = 0xd,
	UART_CMD_BMC_TIME,
	UART_CMD_VISIT_CPLD,
	UART_CMD_CP2112,
};

struct aster_i2c_data
{
    struct device      *hwmon_dev;
    struct mutex        update_lock;
    char                valid;
    unsigned long       last_updated;
    u8  status;
};

enum cx532p_qsfp_sysfs_attributes {
    QSFP_RESET_ALL,
    QSFP_PRESENT_ALL,
};

enum cx532p_lm_sensor_sysfs_attributes {
    LM_SENSOR_NUMBER = 0,
    CPU_L_TEMP = 1,
    CPU_R_TEMP,
    FAN_1_TEMP,
    FAN_2_TEMP,
    TEMP,
};

enum cx532p_core_voltage_sysfs_attributes {
    CORE_VOLTAGE_HIGH = 0,
    CORE_VOLTAGE_LOW,
    SHUTDOWN_DUT,
};

enum cx532p_psu_sysfs_attributes {
    PSU_STATUS = 0,
    PSU1_TYPE,
    PSU1_POWER,
    PSU2_TYPE,
    PSU2_POWER
};

enum cx532p_fan_sysfs_attributes {
    FAN_STATUS,
    FAN_PRESENT,
    FAN_SPEED_RPM,
    FAN_BOARD_TYPE,
};

enum cx532p_eeprom_sysfs_attributes {
	EEPROM_PRODUCT_NAME = 0x21,
	EEPROM_PART_NUMBER,
	EEPROM_SERIAL_NUMBER,
	EEPROM_BASE_MAC_ADDRESS,
	EEPROM_MANUFACTURE_DATA,
	EEPROM_DEVICE_VERSION,
	EEPROM_LABEL_REVISION,
	EEPROM_PLATFORM_NAME,
	EEPROM_ONIE_VERSION,
	EEPROM_MAC_ADDRESSES,
	EEPROM_MANUFACTURER,
	EEPROM_COUNTRY_CODE,
	EEPROM_VENDOR_NAME,
	EEPROM_DIAG_VERSION,
	EEPROM_SERVICE_TAG,
	EEPROM_SWITCH_VENDOR,
	EEPROM_MAIN_BOARD_VERSION,
	EEPROM_COME_VERSION,
	EEPROM_GHC0_BOARD_VERSION,
	EEPROM_GHC1_BOARD_VERSION,
	EEPROM_CRC32 = 0xfe,
};

/* platform dependency defination, reference from serial/8250/8250.h */
struct uart_8250_dma {
        int (*tx_dma)(struct uart_8250_port *p);
        int (*rx_dma)(struct uart_8250_port *p);

        /* Filter function */
        dma_filter_fn           fn;
        /* Parameter to the filter function */
        void                    *rx_param;
        void                    *tx_param;

        struct dma_slave_config rxconf;
        struct dma_slave_config txconf;

        struct dma_chan         *rxchan;
        struct dma_chan         *txchan;

        /* Device address base for DMA operations */
        phys_addr_t             rx_dma_addr;
        phys_addr_t             tx_dma_addr;

        /* DMA address of the buffer in memory */
        dma_addr_t              rx_addr;
        dma_addr_t              tx_addr;

        dma_cookie_t            rx_cookie;
        dma_cookie_t            tx_cookie;

        void                    *rx_buf;

        size_t                  rx_size;
        size_t                  tx_size;

        unsigned char           tx_running;
        unsigned char           tx_err;
        unsigned char           rx_running;
};

struct mid8250;
struct mid8250_board {
	unsigned int flags;
	unsigned long freq;
	unsigned int base_baud;
	int (*setup)(struct mid8250 *, struct uart_port *p);
	void (*exit)(struct mid8250 *);
};

struct mid8250 {
	int line;
	int dma_index;
	struct pci_dev *dma_dev;
	struct uart_8250_dma dma;
	struct mid8250_board *board;
	struct hsu_dma_chip dma_chip;
};
/* end: reference from serial/8250/8250.h */

ssize_t cx532p_bmc_eeprom_value_return(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t cx532p_get_fan_board_type(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t cx532p_fan_status_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t cx532p_fan_presence_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t cx532p_fan_speed_rpm_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t cx532p_fan_direction_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t cx532p_get_psu1_type(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t cx532p_get_psu2_type(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t cx532p_get_psu_status(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t cx532p_get_psu_present(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t cx532p_get_psu_direction(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t cx532p_get_psu_warning(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t cx532p_get_psu_direction_warning(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t cx532p_caculate_power(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t qsfp_reset_all_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t qsfp_reset_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t qsfp_present_all_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t qsfp_present_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t sfp_tx_disable_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t xport_led_mode_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t cx532p_read_core_voltage_high(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t cx532p_read_core_voltage_low(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t cx532p_shutdown_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t cx532p_caculate_temp(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t bmc_version_get(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t cpld_version_get(struct device *dev, struct device_attribute *attr, char *buf);

static SENSOR_DEVICE_ATTR(cpld_version, S_IRUGO, cpld_version_get, NULL, 0);
static SENSOR_DEVICE_ATTR(bmc_version, S_IRUGO, bmc_version_get, NULL, 0);

static SENSOR_DEVICE_ATTR(shutdown_set, S_IRUGO | S_IWUSR,      NULL, cx532p_shutdown_set, SHUTDOWN_DUT);
static SENSOR_DEVICE_ATTR(product_name, S_IRUGO ,				cx532p_bmc_eeprom_value_return  , NULL  , EEPROM_PRODUCT_NAME);
static SENSOR_DEVICE_ATTR(part_number, S_IRUGO,					cx532p_bmc_eeprom_value_return  , NULL  , EEPROM_PART_NUMBER);
static SENSOR_DEVICE_ATTR(serial_number, S_IRUGO,				cx532p_bmc_eeprom_value_return  , NULL  , EEPROM_SERIAL_NUMBER);
static SENSOR_DEVICE_ATTR(base_mac_address, S_IRUGO,			cx532p_bmc_eeprom_value_return  , NULL  , EEPROM_BASE_MAC_ADDRESS);
static SENSOR_DEVICE_ATTR(manufacture_data, S_IRUGO,			cx532p_bmc_eeprom_value_return  , NULL  , EEPROM_MANUFACTURE_DATA);
static SENSOR_DEVICE_ATTR(device_version, S_IRUGO,				cx532p_bmc_eeprom_value_return  , NULL  , EEPROM_DEVICE_VERSION);
static SENSOR_DEVICE_ATTR(lable_revision, S_IRUGO,				cx532p_bmc_eeprom_value_return  , NULL  , EEPROM_LABEL_REVISION);
static SENSOR_DEVICE_ATTR(platform_name, S_IRUGO,				cx532p_bmc_eeprom_value_return  , NULL  , EEPROM_PLATFORM_NAME);
static SENSOR_DEVICE_ATTR(onie_version, S_IRUGO,				cx532p_bmc_eeprom_value_return  , NULL  , EEPROM_ONIE_VERSION);
static SENSOR_DEVICE_ATTR(mac_address, S_IRUGO,					cx532p_bmc_eeprom_value_return  , NULL  , EEPROM_MAC_ADDRESSES);
static SENSOR_DEVICE_ATTR(manufacturer, S_IRUGO,				cx532p_bmc_eeprom_value_return  , NULL  , EEPROM_MANUFACTURER);
static SENSOR_DEVICE_ATTR(country_code, S_IRUGO,				cx532p_bmc_eeprom_value_return  , NULL  , EEPROM_COUNTRY_CODE);
static SENSOR_DEVICE_ATTR(vendor_name, S_IRUGO,					cx532p_bmc_eeprom_value_return  , NULL  , EEPROM_VENDOR_NAME);
static SENSOR_DEVICE_ATTR(diag_version, S_IRUGO,				cx532p_bmc_eeprom_value_return  , NULL  , EEPROM_DIAG_VERSION);
static SENSOR_DEVICE_ATTR(service_tag, S_IRUGO,					cx532p_bmc_eeprom_value_return  , NULL  , EEPROM_SERVICE_TAG);
static SENSOR_DEVICE_ATTR(switch_verdor, S_IRUGO,				cx532p_bmc_eeprom_value_return  , NULL  , EEPROM_SWITCH_VENDOR);
static SENSOR_DEVICE_ATTR(main_board_version, S_IRUGO,			cx532p_bmc_eeprom_value_return  , NULL  , EEPROM_MAIN_BOARD_VERSION);
static SENSOR_DEVICE_ATTR(come_version, S_IRUGO,				cx532p_bmc_eeprom_value_return  , NULL  , EEPROM_COME_VERSION);
static SENSOR_DEVICE_ATTR(ghc0_board_version, S_IRUGO,			cx532p_bmc_eeprom_value_return  , NULL  , EEPROM_GHC0_BOARD_VERSION);
static SENSOR_DEVICE_ATTR(ghc1_board_version, S_IRUGO,			cx532p_bmc_eeprom_value_return  , NULL  , EEPROM_GHC1_BOARD_VERSION);
static SENSOR_DEVICE_ATTR(eeprom_crc32, S_IRUGO,				cx532p_bmc_eeprom_value_return  , NULL  , EEPROM_CRC32);

static SENSOR_DEVICE_ATTR(fan_board_type, S_IRUGO, cx532p_get_fan_board_type, NULL, FAN_BOARD_TYPE);
static SENSOR_DEVICE_ATTR(fan1_status, S_IRUGO, cx532p_fan_status_get, NULL, 1);
static SENSOR_DEVICE_ATTR(fan2_status, S_IRUGO, cx532p_fan_status_get, NULL, 2);
static SENSOR_DEVICE_ATTR(fan3_status, S_IRUGO, cx532p_fan_status_get, NULL, 3);
static SENSOR_DEVICE_ATTR(fan4_status, S_IRUGO, cx532p_fan_status_get, NULL, 4);
static SENSOR_DEVICE_ATTR(fan5_status, S_IRUGO, cx532p_fan_status_get, NULL, 5);
static SENSOR_DEVICE_ATTR(fan1_present, S_IRUGO, cx532p_fan_presence_get, NULL, 1);
static SENSOR_DEVICE_ATTR(fan2_present, S_IRUGO, cx532p_fan_presence_get, NULL, 2);
static SENSOR_DEVICE_ATTR(fan3_present, S_IRUGO, cx532p_fan_presence_get, NULL, 3);
static SENSOR_DEVICE_ATTR(fan4_present, S_IRUGO, cx532p_fan_presence_get, NULL, 4);
static SENSOR_DEVICE_ATTR(fan5_present, S_IRUGO, cx532p_fan_presence_get, NULL, 5);
static SENSOR_DEVICE_ATTR(fan1_speed_rpm, S_IRUGO, cx532p_fan_speed_rpm_get, NULL, 1);
static SENSOR_DEVICE_ATTR(fan2_speed_rpm, S_IRUGO, cx532p_fan_speed_rpm_get, NULL, 2);
static SENSOR_DEVICE_ATTR(fan3_speed_rpm, S_IRUGO, cx532p_fan_speed_rpm_get, NULL, 3);
static SENSOR_DEVICE_ATTR(fan4_speed_rpm, S_IRUGO, cx532p_fan_speed_rpm_get, NULL, 4);
static SENSOR_DEVICE_ATTR(fan5_speed_rpm, S_IRUGO, cx532p_fan_speed_rpm_get, NULL, 5);
static SENSOR_DEVICE_ATTR(fan1_direction, S_IRUGO, cx532p_fan_direction_get, NULL, 1);
static SENSOR_DEVICE_ATTR(fan2_direction, S_IRUGO, cx532p_fan_direction_get, NULL, 2);
static SENSOR_DEVICE_ATTR(fan3_direction, S_IRUGO, cx532p_fan_direction_get, NULL, 3);
static SENSOR_DEVICE_ATTR(fan4_direction, S_IRUGO, cx532p_fan_direction_get, NULL, 4);
static SENSOR_DEVICE_ATTR(fan5_direction, S_IRUGO, cx532p_fan_direction_get, NULL, 5);

static SENSOR_DEVICE_ATTR(psu_status,  S_IRUGO, cx532p_get_psu_status, NULL, PSU_STATUS);
static SENSOR_DEVICE_ATTR(psu_present,  S_IRUGO, cx532p_get_psu_present, NULL, PSU_STATUS);
static SENSOR_DEVICE_ATTR(psu1_type,  S_IRUGO, cx532p_get_psu1_type, NULL, PSU1_TYPE);
static SENSOR_DEVICE_ATTR(psu1_power, S_IRUGO, cx532p_caculate_power, NULL, PSU1_POWER);
static SENSOR_DEVICE_ATTR(psu2_type,  S_IRUGO, cx532p_get_psu2_type, NULL, PSU2_TYPE);
static SENSOR_DEVICE_ATTR(psu2_power, S_IRUGO, cx532p_caculate_power, NULL, PSU2_POWER);
static SENSOR_DEVICE_ATTR(psu_direction,  S_IRUGO, cx532p_get_psu_direction, NULL, PSU_STATUS);
static SENSOR_DEVICE_ATTR(psu_warning,  S_IRUGO, cx532p_get_psu_warning, NULL, PSU_STATUS);
static SENSOR_DEVICE_ATTR(psu_direction_warning,  S_IRUGO, cx532p_get_psu_direction_warning, NULL, PSU_STATUS);

static SENSOR_DEVICE_ATTR(qsfp_reset_all        , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_all_set       , QSFP_RESET_ALL);
static SENSOR_DEVICE_ATTR(qsfp1_reset           , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 1);
static SENSOR_DEVICE_ATTR(qsfp2_reset           , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 2);
static SENSOR_DEVICE_ATTR(qsfp3_reset           , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 3);
static SENSOR_DEVICE_ATTR(qsfp4_reset           , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 4);
static SENSOR_DEVICE_ATTR(qsfp5_reset           , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 5);
static SENSOR_DEVICE_ATTR(qsfp6_reset           , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 6);
static SENSOR_DEVICE_ATTR(qsfp7_reset           , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 7);
static SENSOR_DEVICE_ATTR(qsfp8_reset           , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 8);
static SENSOR_DEVICE_ATTR(qsfp9_reset           , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 9);
static SENSOR_DEVICE_ATTR(qsfp10_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 10);
static SENSOR_DEVICE_ATTR(qsfp11_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 11);
static SENSOR_DEVICE_ATTR(qsfp12_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 12);
static SENSOR_DEVICE_ATTR(qsfp13_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 13);
static SENSOR_DEVICE_ATTR(qsfp14_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 14);
static SENSOR_DEVICE_ATTR(qsfp15_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 15);
static SENSOR_DEVICE_ATTR(qsfp16_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 16);
static SENSOR_DEVICE_ATTR(qsfp17_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 17);
static SENSOR_DEVICE_ATTR(qsfp18_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 18);
static SENSOR_DEVICE_ATTR(qsfp19_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 19);
static SENSOR_DEVICE_ATTR(qsfp20_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 20);
static SENSOR_DEVICE_ATTR(qsfp21_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 21);
static SENSOR_DEVICE_ATTR(qsfp22_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 22);
static SENSOR_DEVICE_ATTR(qsfp23_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 23);
static SENSOR_DEVICE_ATTR(qsfp24_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 24);
static SENSOR_DEVICE_ATTR(qsfp25_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 25);
static SENSOR_DEVICE_ATTR(qsfp26_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 26);
static SENSOR_DEVICE_ATTR(qsfp27_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 27);
static SENSOR_DEVICE_ATTR(qsfp28_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 28);
static SENSOR_DEVICE_ATTR(qsfp29_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 29);
static SENSOR_DEVICE_ATTR(qsfp30_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 30);
static SENSOR_DEVICE_ATTR(qsfp31_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 31);
static SENSOR_DEVICE_ATTR(qsfp32_reset          , S_IRUGO | S_IWUSR , NULL                     , qsfp_reset_set           , 32);
static SENSOR_DEVICE_ATTR(qsfp_present_all      , S_IRUGO           , qsfp_present_all_get     , NULL                     , QSFP_PRESENT_ALL);
static SENSOR_DEVICE_ATTR(qsfp1_present         , S_IRUGO           , qsfp_present_get         , NULL                     , 1);
static SENSOR_DEVICE_ATTR(qsfp2_present         , S_IRUGO           , qsfp_present_get         , NULL                     , 2);
static SENSOR_DEVICE_ATTR(qsfp3_present         , S_IRUGO           , qsfp_present_get         , NULL                     , 3);
static SENSOR_DEVICE_ATTR(qsfp4_present         , S_IRUGO           , qsfp_present_get         , NULL                     , 4);
static SENSOR_DEVICE_ATTR(qsfp5_present         , S_IRUGO           , qsfp_present_get         , NULL                     , 5);
static SENSOR_DEVICE_ATTR(qsfp6_present         , S_IRUGO           , qsfp_present_get         , NULL                     , 6);
static SENSOR_DEVICE_ATTR(qsfp7_present         , S_IRUGO           , qsfp_present_get         , NULL                     , 7);
static SENSOR_DEVICE_ATTR(qsfp8_present         , S_IRUGO           , qsfp_present_get         , NULL                     , 8);
static SENSOR_DEVICE_ATTR(qsfp9_present         , S_IRUGO           , qsfp_present_get         , NULL                     , 9);
static SENSOR_DEVICE_ATTR(qsfp10_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 10);
static SENSOR_DEVICE_ATTR(qsfp11_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 11);
static SENSOR_DEVICE_ATTR(qsfp12_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 12);
static SENSOR_DEVICE_ATTR(qsfp13_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 13);
static SENSOR_DEVICE_ATTR(qsfp14_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 14);
static SENSOR_DEVICE_ATTR(qsfp15_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 15);
static SENSOR_DEVICE_ATTR(qsfp16_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 16);
static SENSOR_DEVICE_ATTR(qsfp17_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 17);
static SENSOR_DEVICE_ATTR(qsfp18_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 18);
static SENSOR_DEVICE_ATTR(qsfp19_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 19);
static SENSOR_DEVICE_ATTR(qsfp20_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 20);
static SENSOR_DEVICE_ATTR(qsfp21_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 21);
static SENSOR_DEVICE_ATTR(qsfp22_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 22);
static SENSOR_DEVICE_ATTR(qsfp23_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 23);
static SENSOR_DEVICE_ATTR(qsfp24_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 24);
static SENSOR_DEVICE_ATTR(qsfp25_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 25);
static SENSOR_DEVICE_ATTR(qsfp26_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 26);
static SENSOR_DEVICE_ATTR(qsfp27_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 27);
static SENSOR_DEVICE_ATTR(qsfp28_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 28);
static SENSOR_DEVICE_ATTR(qsfp29_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 29);
static SENSOR_DEVICE_ATTR(qsfp30_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 30);
static SENSOR_DEVICE_ATTR(qsfp31_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 31);
static SENSOR_DEVICE_ATTR(qsfp32_present        , S_IRUGO           , qsfp_present_get         , NULL                     , 32);
static SENSOR_DEVICE_ATTR(sfp1_present          , S_IRUGO           , qsfp_present_get         , NULL                     , 33);
static SENSOR_DEVICE_ATTR(sfp2_present          , S_IRUGO           , qsfp_present_get         , NULL                     , 34);
static SENSOR_DEVICE_ATTR(sfp1_tx_disable       , S_IRUGO | S_IWUSR , NULL                     , sfp_tx_disable_set       , 1);
static SENSOR_DEVICE_ATTR(sfp2_tx_disable       , S_IRUGO | S_IWUSR , NULL                     , sfp_tx_disable_set       , 2);
static SENSOR_DEVICE_ATTR(xport_led_mode        , S_IRUGO | S_IWUSR , NULL                     , xport_led_mode_set       , 0);

static SENSOR_DEVICE_ATTR(core_voltage_high , S_IRUGO , cx532p_read_core_voltage_high , NULL , CORE_VOLTAGE_HIGH);
static SENSOR_DEVICE_ATTR(core_voltage_low , S_IRUGO , cx532p_read_core_voltage_low , NULL , CORE_VOLTAGE_LOW);

static SENSOR_DEVICE_ATTR(lm_sensor_number,               S_IRUGO, cx532p_caculate_temp,             NULL, LM_SENSOR_NUMBER);
static SENSOR_DEVICE_ATTR(cpu_l_temp,                     S_IRUGO, cx532p_caculate_temp,             NULL, CPU_L_TEMP);
static SENSOR_DEVICE_ATTR(cpu_r_temp,                     S_IRUGO, cx532p_caculate_temp,             NULL, CPU_R_TEMP);
static SENSOR_DEVICE_ATTR(fan_1_temp,                     S_IRUGO, cx532p_caculate_temp,             NULL, FAN_1_TEMP);
static SENSOR_DEVICE_ATTR(fan_2_temp,                     S_IRUGO, cx532p_caculate_temp,             NULL, FAN_2_TEMP);
static SENSOR_DEVICE_ATTR(temp,                      S_IRUGO, cx532p_caculate_temp,             NULL, TEMP);

static struct attribute *sys_info_attrbutes[] = {
    &sensor_dev_attr_bmc_version.dev_attr.attr,
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    NULL
};

static struct attribute *cx532p_eeprom_attrbutes[] = {
    &sensor_dev_attr_product_name.dev_attr.attr,
	&sensor_dev_attr_part_number.dev_attr.attr,
	&sensor_dev_attr_serial_number.dev_attr.attr,
	&sensor_dev_attr_base_mac_address.dev_attr.attr,
	&sensor_dev_attr_manufacture_data.dev_attr.attr,
	&sensor_dev_attr_device_version.dev_attr.attr,
	&sensor_dev_attr_lable_revision.dev_attr.attr,
	&sensor_dev_attr_platform_name.dev_attr.attr,
	&sensor_dev_attr_onie_version.dev_attr.attr,
	&sensor_dev_attr_mac_address.dev_attr.attr,
	&sensor_dev_attr_manufacturer.dev_attr.attr,
	&sensor_dev_attr_country_code.dev_attr.attr,
	&sensor_dev_attr_vendor_name.dev_attr.attr,
	&sensor_dev_attr_diag_version.dev_attr.attr,
	&sensor_dev_attr_service_tag.dev_attr.attr,
	&sensor_dev_attr_switch_verdor.dev_attr.attr,
	&sensor_dev_attr_main_board_version.dev_attr.attr,
	&sensor_dev_attr_come_version.dev_attr.attr,
	&sensor_dev_attr_ghc0_board_version.dev_attr.attr,
	&sensor_dev_attr_ghc1_board_version.dev_attr.attr,
	&sensor_dev_attr_eeprom_crc32.dev_attr.attr,
    NULL
};

static struct attribute *cx532p_fan_attrbutes[] = {
    &sensor_dev_attr_fan_board_type.dev_attr.attr,

    &sensor_dev_attr_fan1_status.dev_attr.attr,
    &sensor_dev_attr_fan2_status.dev_attr.attr,
    &sensor_dev_attr_fan3_status.dev_attr.attr,
    &sensor_dev_attr_fan4_status.dev_attr.attr,
    &sensor_dev_attr_fan5_status.dev_attr.attr,
    &sensor_dev_attr_fan1_present.dev_attr.attr,
    &sensor_dev_attr_fan2_present.dev_attr.attr,
    &sensor_dev_attr_fan3_present.dev_attr.attr,
    &sensor_dev_attr_fan4_present.dev_attr.attr,
    &sensor_dev_attr_fan5_present.dev_attr.attr,
    &sensor_dev_attr_fan1_speed_rpm.dev_attr.attr,
    &sensor_dev_attr_fan2_speed_rpm.dev_attr.attr,
    &sensor_dev_attr_fan3_speed_rpm.dev_attr.attr,
    &sensor_dev_attr_fan4_speed_rpm.dev_attr.attr,
    &sensor_dev_attr_fan5_speed_rpm.dev_attr.attr,
    &sensor_dev_attr_fan1_direction.dev_attr.attr,
    &sensor_dev_attr_fan2_direction.dev_attr.attr,
    &sensor_dev_attr_fan3_direction.dev_attr.attr,
    &sensor_dev_attr_fan4_direction.dev_attr.attr,
    &sensor_dev_attr_fan5_direction.dev_attr.attr,
    NULL
};

static struct attribute *cx532p_psu_attrbutes[] = {
    &sensor_dev_attr_psu_status.dev_attr.attr,
    &sensor_dev_attr_psu_present.dev_attr.attr,
    &sensor_dev_attr_psu1_type.dev_attr.attr,
    &sensor_dev_attr_psu1_power.dev_attr.attr,
    &sensor_dev_attr_psu2_type.dev_attr.attr,
    &sensor_dev_attr_psu2_power.dev_attr.attr,
    &sensor_dev_attr_psu_direction.dev_attr.attr,
    &sensor_dev_attr_psu_warning.dev_attr.attr,
    &sensor_dev_attr_psu_direction_warning.dev_attr.attr,
    NULL
};

static struct attribute *cx532p_qsfp_attrbutes[] = {
    &sensor_dev_attr_qsfp_reset_all.dev_attr.attr,
    &sensor_dev_attr_qsfp1_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp2_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp3_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp4_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp5_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp6_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp7_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp8_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp9_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp10_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp11_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp12_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp13_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp14_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp15_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp16_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp17_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp18_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp19_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp20_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp21_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp22_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp23_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp24_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp25_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp26_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp27_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp28_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp29_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp30_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp31_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp32_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp_present_all.dev_attr.attr,
    &sensor_dev_attr_qsfp1_present.dev_attr.attr,
    &sensor_dev_attr_qsfp2_present.dev_attr.attr,
    &sensor_dev_attr_qsfp3_present.dev_attr.attr,
    &sensor_dev_attr_qsfp4_present.dev_attr.attr,
    &sensor_dev_attr_qsfp5_present.dev_attr.attr,
    &sensor_dev_attr_qsfp6_present.dev_attr.attr,
    &sensor_dev_attr_qsfp7_present.dev_attr.attr,
    &sensor_dev_attr_qsfp8_present.dev_attr.attr,
    &sensor_dev_attr_qsfp9_present.dev_attr.attr,
    &sensor_dev_attr_qsfp10_present.dev_attr.attr,
    &sensor_dev_attr_qsfp11_present.dev_attr.attr,
    &sensor_dev_attr_qsfp12_present.dev_attr.attr,
    &sensor_dev_attr_qsfp13_present.dev_attr.attr,
    &sensor_dev_attr_qsfp14_present.dev_attr.attr,
    &sensor_dev_attr_qsfp15_present.dev_attr.attr,
    &sensor_dev_attr_qsfp16_present.dev_attr.attr,
    &sensor_dev_attr_qsfp17_present.dev_attr.attr,
    &sensor_dev_attr_qsfp18_present.dev_attr.attr,
    &sensor_dev_attr_qsfp19_present.dev_attr.attr,
    &sensor_dev_attr_qsfp20_present.dev_attr.attr,
    &sensor_dev_attr_qsfp21_present.dev_attr.attr,
    &sensor_dev_attr_qsfp22_present.dev_attr.attr,
    &sensor_dev_attr_qsfp23_present.dev_attr.attr,
    &sensor_dev_attr_qsfp24_present.dev_attr.attr,
    &sensor_dev_attr_qsfp25_present.dev_attr.attr,
    &sensor_dev_attr_qsfp26_present.dev_attr.attr,
    &sensor_dev_attr_qsfp27_present.dev_attr.attr,
    &sensor_dev_attr_qsfp28_present.dev_attr.attr,
    &sensor_dev_attr_qsfp29_present.dev_attr.attr,
    &sensor_dev_attr_qsfp30_present.dev_attr.attr,
    &sensor_dev_attr_qsfp31_present.dev_attr.attr,
    &sensor_dev_attr_qsfp32_present.dev_attr.attr,
    &sensor_dev_attr_sfp1_present.dev_attr.attr,
    &sensor_dev_attr_sfp2_present.dev_attr.attr,
    &sensor_dev_attr_sfp1_tx_disable.dev_attr.attr,
    &sensor_dev_attr_sfp2_tx_disable.dev_attr.attr,
    &sensor_dev_attr_xport_led_mode.dev_attr.attr,
    NULL
};

static struct attribute *cx532p_core_voltage_attrbutes[] = {
    &sensor_dev_attr_core_voltage_high.dev_attr.attr,
    &sensor_dev_attr_core_voltage_low.dev_attr.attr,
    &sensor_dev_attr_shutdown_set.dev_attr.attr,
    NULL
};

static struct attribute *cx532p_lm_sensor_attrbutes[] = {
    &sensor_dev_attr_lm_sensor_number.dev_attr.attr,
    &sensor_dev_attr_cpu_l_temp.dev_attr.attr,
    &sensor_dev_attr_cpu_r_temp.dev_attr.attr,
    &sensor_dev_attr_fan_1_temp.dev_attr.attr,
    &sensor_dev_attr_fan_2_temp.dev_attr.attr,
    &sensor_dev_attr_temp.dev_attr.attr,
    NULL
};

static const struct attribute_group sys_info_group = {
    .name  = "SYS_INFO",
    .attrs = sys_info_attrbutes,
};

static const struct attribute_group cx532p_eeprom_group = {
    .name  = "SYS_EEPROM",
    .attrs = cx532p_eeprom_attrbutes,
};

static const struct attribute_group cx532p_fan_group = {
    .name  = "CX532P_FAN",
    .attrs = cx532p_fan_attrbutes,
};

static const struct attribute_group cx532p_psu_group = {
    .name  = "CX532P_PSU",
    .attrs = cx532p_psu_attrbutes,
};

static const struct attribute_group cx532p_qsfp_group = {
    .name  = "CX532P_QSFP",
    .attrs = cx532p_qsfp_attrbutes,
};

static const struct attribute_group cx532p_core_voltage_group = {
    .name  = "CX532P_SYS",
    .attrs = cx532p_core_voltage_attrbutes,
};

static const struct attribute_group cx532p_lm_sensor_group = {
    .name  = "CX532P_THERMAL",
    .attrs = cx532p_lm_sensor_attrbutes,
};

#endif
