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

#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <asm/irq.h>
#include <linux/serial_reg.h>	// for macros related to serial register

#define DRIVER_VERSION  "1.3"

#define TURN_OFF        0
#define TURN_ON         1
#define GET_USB         2
#define GET_LOC         3
#define LOC_OFF         0
#define LOC_BLINK       1
#define ALARM_OFF       0
#define ALARM_AMBER     1
#define ALARM_GREEN     2
#define PSU_1_GOOD      3
#define PSU_2_GOOD      4
#define PCIE_INT        1
#define QSFP_1_INT      2
#define QSFP_2_INT      3
#define FAN_INT         4
#define PSU_INT         5
#define SENSOR_INT      6
#define USB_INT         7
#define USB_ON          0x2
#define USB_OFF         0xfd
#define DIAG_G_ON       0x2
#define DIAG_G_OFF      0xfd
#define LED_ON          0x1
#define LED_OFF         0xfe
#define DIAG_A_ON       0x1
#define DIAG_A_OFF      0xfe
#define LOC_LED_OFF     0x4
#define LOC_LED_BLINK   0xfb
#define SWITCH_LED_OFF  0
#define SWITCH_LED_A_N  1
#define SWITCH_LED_A_B  2
#define SWITCH_LED_G_N  3
#define SWITCH_LED_G_B  4

#define BMC_PRESENT_OFFSET 0xa4

#define UART_TTYS1_OFFSET 	0x2f8
#define UART_TTYS1_SIRQ		3

#define SYSFAN_MAX_NUM  4

#define UART_MAX_SIZE 128

struct i2c_adap {
	int nr;
	char *name;
	const char *funcs;
	const char *algo;
};

struct i2c_adap *gather_i2c_busses(void);
void free_adapters(struct i2c_adap *adapters);

/* compiler conditional */
// #define LED_CTRL_WANTED
// #define USB_CTRL_WANTED

#ifdef DEBUG_MSG
    #define debug_print(s) printk s
#else
    #define debug_print(s)
#endif

/* end of compiler conditional */

/* Function Declaration */
ssize_t psu_status_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t psu_module_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t psu_direction_get(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t psu_warning_get(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t psu_direction_warning_get(struct device *dev, struct device_attribute *attr, char *buf);

ssize_t sfp_status_get(struct device *dev, struct device_attribute *da, char *buf);
ssize_t sfp_tx_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);

ssize_t qsfp_reset_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
ssize_t qsfp_status_get(struct device *dev, struct device_attribute *da, char *buf);

ssize_t fan_status_get(struct device *dev, struct device_attribute *da, char *buf);

ssize_t thermal_get(struct device *dev, struct device_attribute *da, char *buf);

ssize_t eeprom_get(struct device *dev, struct device_attribute *da, char *buf);

ssize_t bmc_version_get(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t cpld_version_get(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t clk_sel_get(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t clk_sel_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
/* end of Function Declaration */

/* struct i2c_data */
struct Asterfusion_i2c_data
{
    struct device      *hwmon_dev;
    struct mutex        update_lock;
    char                valid;          /* !=0 if registers are valid */
    unsigned long       last_updated;   /* In jiffies */
    u8  status;                         /* Status register read from CPLD */
};

/* struct uart_data */
struct Asterfusion_uart_data
{
	u16					iobase;
	u16					sirq;
    struct mutex        update_lock;
    char                valid;          /* !=0 if registers are valid */
};


/* struct i2c_sysfs_attributes */
enum Asterfusion_i2c_sysfs_attributes
{
    PSU_PRESENT,
    PSU_STATUS,
    PSU_MODULE_1,
    PSU_MODULE_2,
	PSU_DIRECTION,
	PSU_WARNING,
	PSU_DIRECTION_WARNING,
    DC_CHIP_SWITCH,

#ifdef USB_CTRL_WANTED
    USB_POWER,
#endif
#ifdef LED_CTRL_WANTED
    LED_CTRL,
#endif
    SYS_LED,
    FLOW_LED,
    SW_LED_1,
    SW_LED_2,
    RESET_MAC,
    SHUTDOWN_DUT,
    SENSOR_STATUS,
    SENSOR_TEMP,
    SENSOR_INT_MASK,
    INT_STATUS,
    SFP_PRESENT,
    SFP_RX_LOSS,
    SFP_TX_STAT,
    QSFP_LOW_POWER_ALL,
    QSFP_RESET,
    QSFP_PRESENT,
    QSFP_INT,
    FAN_STATUS,
    FAN_PRESENT,
    FAN_AIRFLOW,
    FAN_SPEED_RPM,
    THERMAL_SERSOR_1,
    THERMAL_SERSOR_2,
    THERMAL_SERSOR_3,
    THERMAL_SERSOR_4,
    BMC_DETECT,
#ifdef WDT_CTRL_WANTED
    WDT_CTRL,
#endif
#ifdef EEPROM_WP_WANTED
    EEPROM_WP_CTRL,
#endif
    HW_VER,
};
/* end of struct i2c_sysfs_attributes */

enum Asterfusion_uart_cmd_attributes{
	UART_CMD_EEPROM = 0x1,
	UART_CMD_TEMP = 0x4,
	UART_CMD_FAN_INFO,
	UART_CMD_FAN_STATUS,
	UART_CMD_FAN_OP,
	UART_CMD_PAYLOAD_INFO,
	UART_CMD_THERMAL_OP,
	UART_CMD_PAYLOAD_OP,
	UART_CMD_PSU,
	UART_CMD_BMC_VERSION = 0xd,
	UART_CMD_BMC_TIME,
	UART_CMD_VISIT_CPLD,
	UART_CMD_CP2112,
};

enum Asterfusion_uart_subcmd_attributes{
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

	FAN_STATUS_1 = 0x1,
	FAN_STATUS_2 = 0x2,
	FAN_STATUS_3 = 0x3,
	FAN_STATUS_4 = 0x4,
	FAN_GET_SPEED = 0x0,
	FAN_GET_STATUS = 0x1,
	FAN_OPEN_AUTO_SPEED = 0x0,
	FAN_CLOSE_AUTO_SPEED = 0x1,

	FAN_SPEED_LEVEL_1 = 0x1,
	FAN_SPEED_LEVEL_2 = 0x2,
	FAN_SPEED_LEVEL_3 = 0x3,
	FAN_SPEED_LEVEL_4 = 0x4,
	FAN_SPEED_LEVEL_5 = 0x5,
	FAN_SPEED_LEVEL_6 = 0x6,
	FAN_SPEED_LEVEL_7 = 0x7,
	FAN_SPEED_LEVEL_8 = 0x8,
	FAN_SPEED_LEVEL_9 = 0x9,
	FAN_SPEED_LEVEL_10 = 0xa,

	PAYLOAD_SHUTDOWN = 0xaa,
	PAYLOAD_REBOOT = 0xa1,

	PSU_OP_STATUS = 0x0,
	PSU_OP_INFO = 0x1,

	BMC_GET_TIME = 0x1,
	BMC_SET_TIME = 0x2,

	VISIT_CPLD_BY_CP2112 = 0x1,
	VISIT_CPLD_BY_SIO = 0x2,

	RELAY_FAN_CP2112 = 0x1,
	RELAY_CME_CPLD_CP2112 = 0x2,

	SUBCMD_DEFAULT = 0xaa,

};


/* sysfs attributes for SENSOR_DEVICE_ATTR */
static SENSOR_DEVICE_ATTR(cpld_version, S_IRUGO, cpld_version_get, NULL, 0);
static SENSOR_DEVICE_ATTR(bmc_version, S_IRUGO, bmc_version_get, NULL, 0);
static SENSOR_DEVICE_ATTR(clk_sel           , S_IRUGO | S_IWUSR , clk_sel_get       , clk_sel_set       , 0);

static SENSOR_DEVICE_ATTR(psu_present       , S_IRUGO           , psu_status_get    , NULL              , PSU_PRESENT);
static SENSOR_DEVICE_ATTR(psu_status        , S_IRUGO           , psu_status_get    , NULL              , PSU_STATUS);
static SENSOR_DEVICE_ATTR(psu_module_1      , S_IRUGO           , psu_module_get    , NULL              , PSU_MODULE_1);
static SENSOR_DEVICE_ATTR(psu_module_2      , S_IRUGO           , psu_module_get    , NULL              , PSU_MODULE_2);
static SENSOR_DEVICE_ATTR(psu_direction		, S_IRUGO			, psu_direction_get	, NULL				, PSU_DIRECTION);
static SENSOR_DEVICE_ATTR(psu_warning		, S_IRUGO			, psu_warning_get	, NULL				, PSU_WARNING);
static SENSOR_DEVICE_ATTR(psu_direction_warning	,  S_IRUGO		, psu_direction_warning_get	, NULL		, PSU_DIRECTION_WARNING);

static SENSOR_DEVICE_ATTR(SFP_present       , S_IRUGO           , sfp_status_get    , NULL              , SFP_PRESENT);
static SENSOR_DEVICE_ATTR(SFP_rx_loss       , S_IRUGO           , sfp_status_get    , NULL              , SFP_RX_LOSS);
static SENSOR_DEVICE_ATTR(SFP_tx_stat       , S_IRUGO           , sfp_status_get    , NULL              , SFP_TX_STAT);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_1     , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 1);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_2     , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 2);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_3     , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 3);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_4     , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 4);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_5     , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 5);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_6     , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 6);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_7     , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 7);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_8     , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 8);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_9     , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 9);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_10    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 10);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_11    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 11);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_12    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 12);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_13    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 13);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_14    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 14);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_15    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 15);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_16    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 16);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_17    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 17);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_18    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 18);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_19    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 19);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_20    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 20);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_21    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 21);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_22    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 22);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_23    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 23);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_24    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 24);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_25    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 25);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_26    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 26);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_27    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 27);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_28    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 28);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_29    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 29);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_30    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 30);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_31    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 31);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_32    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 32);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_33    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 33);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_34    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 34);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_35    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 35);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_36    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 36);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_37    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 37);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_38    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 38);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_39    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 39);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_40    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 40);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_41    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 41);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_42    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 42);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_43    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 43);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_44    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 44);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_45    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 45);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_46    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 46);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_47    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 47);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_48    , S_IRUGO | S_IWUSR , NULL              , sfp_tx_set        , 48);

static SENSOR_DEVICE_ATTR(QSFP_reset        , S_IRUGO | S_IWUSR , NULL              , qsfp_reset_set    , QSFP_RESET);
static SENSOR_DEVICE_ATTR(QSFP_present      , S_IRUGO           , qsfp_status_get   , NULL              , QSFP_PRESENT);
static SENSOR_DEVICE_ATTR(QSFP_int          , S_IRUGO           , qsfp_status_get   , NULL              , QSFP_INT);

static SENSOR_DEVICE_ATTR(fan_status        , S_IRUGO           , fan_status_get    , NULL              , FAN_STATUS);
static SENSOR_DEVICE_ATTR(fan_present       , S_IRUGO           , fan_status_get    , NULL              , FAN_PRESENT);
static SENSOR_DEVICE_ATTR(fan_airflow       , S_IRUGO           , fan_status_get    , NULL              , FAN_AIRFLOW);
static SENSOR_DEVICE_ATTR(fan_speed_rpm     , S_IRUGO           , fan_status_get    , NULL              , FAN_SPEED_RPM);

static SENSOR_DEVICE_ATTR(thermal_sersor_1      , S_IRUGO           , thermal_get  , NULL              , THERMAL_SERSOR_1);
static SENSOR_DEVICE_ATTR(thermal_sersor_2      , S_IRUGO           , thermal_get  , NULL              , THERMAL_SERSOR_2);
static SENSOR_DEVICE_ATTR(thermal_sersor_3      , S_IRUGO           , thermal_get  , NULL              , THERMAL_SERSOR_3);
static SENSOR_DEVICE_ATTR(thermal_sersor_4      , S_IRUGO           , thermal_get  , NULL              , THERMAL_SERSOR_4);

static SENSOR_DEVICE_ATTR(product_name, S_IRUGO ,				eeprom_get  , NULL  , EEPROM_PRODUCT_NAME);
static SENSOR_DEVICE_ATTR(part_number, S_IRUGO,					eeprom_get  , NULL  , EEPROM_PART_NUMBER);
static SENSOR_DEVICE_ATTR(serial_number, S_IRUGO,				eeprom_get  , NULL  , EEPROM_SERIAL_NUMBER);
static SENSOR_DEVICE_ATTR(base_mac_address, S_IRUGO,			eeprom_get  , NULL  , EEPROM_BASE_MAC_ADDRESS);
static SENSOR_DEVICE_ATTR(manufacture_data, S_IRUGO,			eeprom_get  , NULL  , EEPROM_MANUFACTURE_DATA);
static SENSOR_DEVICE_ATTR(device_version, S_IRUGO,				eeprom_get  , NULL  , EEPROM_DEVICE_VERSION);
static SENSOR_DEVICE_ATTR(lable_revision, S_IRUGO,				eeprom_get  , NULL  , EEPROM_LABEL_REVISION);
static SENSOR_DEVICE_ATTR(platform_name, S_IRUGO,				eeprom_get  , NULL  , EEPROM_PLATFORM_NAME);
static SENSOR_DEVICE_ATTR(onie_version, S_IRUGO,				eeprom_get  , NULL  , EEPROM_ONIE_VERSION);
static SENSOR_DEVICE_ATTR(mac_address, S_IRUGO,					eeprom_get  , NULL  , EEPROM_MAC_ADDRESSES);
static SENSOR_DEVICE_ATTR(manufacturer, S_IRUGO,				eeprom_get  , NULL  , EEPROM_MANUFACTURER);
static SENSOR_DEVICE_ATTR(country_code, S_IRUGO,				eeprom_get  , NULL  , EEPROM_COUNTRY_CODE);
static SENSOR_DEVICE_ATTR(vendor_name, S_IRUGO,					eeprom_get  , NULL  , EEPROM_VENDOR_NAME);
static SENSOR_DEVICE_ATTR(diag_version, S_IRUGO,				eeprom_get  , NULL  , EEPROM_DIAG_VERSION);
static SENSOR_DEVICE_ATTR(service_tag, S_IRUGO,					eeprom_get  , NULL  , EEPROM_SERVICE_TAG);
static SENSOR_DEVICE_ATTR(switch_verdor, S_IRUGO,				eeprom_get  , NULL  , EEPROM_SWITCH_VENDOR);
static SENSOR_DEVICE_ATTR(main_board_version, S_IRUGO,			eeprom_get  , NULL  , EEPROM_MAIN_BOARD_VERSION);
static SENSOR_DEVICE_ATTR(come_version, S_IRUGO,				eeprom_get  , NULL  , EEPROM_COME_VERSION);
static SENSOR_DEVICE_ATTR(ghc0_board_version, S_IRUGO,			eeprom_get  , NULL  , EEPROM_GHC0_BOARD_VERSION);
static SENSOR_DEVICE_ATTR(ghc1_board_version, S_IRUGO,			eeprom_get  , NULL  , EEPROM_GHC1_BOARD_VERSION);
static SENSOR_DEVICE_ATTR(eeprom_crc32, S_IRUGO,				eeprom_get  , NULL  , EEPROM_CRC32);
/* end of sysfs attributes for SENSOR_DEVICE_ATTR */

/* sysfs attributes for hwmon */
static struct attribute *sys_info_attrbutes[] = {
    &sensor_dev_attr_bmc_version.dev_attr.attr,
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    &sensor_dev_attr_clk_sel.dev_attr.attr,
    NULL
};

static struct attribute *CX308P_PSU_attributes[] =
{
    &sensor_dev_attr_psu_present.dev_attr.attr,
    &sensor_dev_attr_psu_status.dev_attr.attr,
    &sensor_dev_attr_psu_module_1.dev_attr.attr,
    &sensor_dev_attr_psu_module_2.dev_attr.attr,
    &sensor_dev_attr_psu_direction.dev_attr.attr,
    &sensor_dev_attr_psu_warning.dev_attr.attr,
    &sensor_dev_attr_psu_direction_warning.dev_attr.attr,
    NULL
};

static struct attribute *CX308P_INT_attributes[] =
{
    &sensor_dev_attr_QSFP_int.dev_attr.attr,
    NULL
};

static struct attribute *CX308P_SFP_attributes[] =
{
    &sensor_dev_attr_SFP_present.dev_attr.attr,
    &sensor_dev_attr_SFP_rx_loss.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_stat.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_1.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_2.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_3.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_4.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_5.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_6.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_7.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_8.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_9.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_10.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_11.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_12.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_13.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_14.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_15.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_16.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_17.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_18.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_19.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_20.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_21.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_22.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_23.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_24.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_25.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_26.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_27.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_28.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_29.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_30.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_31.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_32.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_33.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_34.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_35.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_36.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_37.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_38.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_39.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_40.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_41.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_42.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_43.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_44.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_45.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_46.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_47.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_48.dev_attr.attr,
    NULL
};

static struct attribute *CX308P_QSFP_attributes[] =
{
    &sensor_dev_attr_QSFP_present.dev_attr.attr,
    &sensor_dev_attr_QSFP_reset.dev_attr.attr,
    NULL
};

static struct attribute *CX308P_FAN_attributes[] = {
    &sensor_dev_attr_fan_status.dev_attr.attr,
    &sensor_dev_attr_fan_present.dev_attr.attr,
    &sensor_dev_attr_fan_airflow.dev_attr.attr,
    &sensor_dev_attr_fan_speed_rpm.dev_attr.attr,
    NULL
};

static struct attribute *CX308P_THERMAL_attributes[] = {
    &sensor_dev_attr_thermal_sersor_1.dev_attr.attr,
    &sensor_dev_attr_thermal_sersor_2.dev_attr.attr,
    &sensor_dev_attr_thermal_sersor_3.dev_attr.attr,
    &sensor_dev_attr_thermal_sersor_4.dev_attr.attr,
    NULL
};

static struct attribute *CX308P_EEPROM_attributes[] =
{
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
/* end of sysfs attributes for hwmon */

/* struct attribute_group */
static const struct attribute_group sys_info_group = {
    .name  = "SYS_INFO",
    .attrs = sys_info_attrbutes,
};

static const struct attribute_group CX308P_PSU_group =
{
    .name  = "CX308P_PSU",
    .attrs = CX308P_PSU_attributes,
};

static const struct attribute_group CX308P_INT_group =
{
    .name  = "CX308P_INT",
    .attrs = CX308P_INT_attributes,
};

static const struct attribute_group CX308P_SFP_group =
{
    .name  = "CX308P_SFP",
    .attrs = CX308P_SFP_attributes,
};

static const struct attribute_group CX308P_QSFP_group =
{
    .name  = "CX308P_QSFP",
    .attrs = CX308P_QSFP_attributes,
};

static const struct attribute_group CX308P_FAN_group =
{
    .name  = "CX308P_FAN",
    .attrs = CX308P_FAN_attributes,
};

static const struct attribute_group CX308P_THERMAL_group =
{
    .name  = "CX308P_THERMAL",
    .attrs = CX308P_THERMAL_attributes,
};

static const struct attribute_group CX308P_EEPROM_group =
{
	.name  = "SYS_EEPROM",
	.attrs = CX308P_EEPROM_attributes,
};
/* end of struct attribute_group */
