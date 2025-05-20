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

#define SFP54_BIT   5
#define SFP53_BIT   4
#define SFP52_BIT   3
#define SFP51_BIT   2
#define SFP50_BIT   1
#define SFP49_BIT   0

#define PCA9548_CTL_BIT 6
#define PHY6_CTL_BIT    5
#define PHY5_CTL_BIT    4
#define PHY4_CTL_BIT    3
#define PHY3_CTL_BIT    2
#define PHY2_CTL_BIT    1
#define PHY1_CTL_BIT    0

#define EEPROM_WP_CTRL_BIT  7
#define OVER_TEMP_CTRL_BIT  1
#define WDT_CTRL_BIT        0

#define FAN3_STATUS_BIT     5
#define FAN2_STATUS_BIT     4
#define FAN1_STATUS_BIT     3
#define FAN3_PRESNET_BIT    2
#define FAN2_PRESNET_BIT    1
#define FAN1_PRESNET_BIT    0

#define PSU2_POWER_BIT      5
#define PSU1_POWER_BIT      4
#define PSU2_ALERT_BIT      3
#define PSU1_ALERT_BIT      2
#define PSU2_PRESNET_BIT    1
#define PSU1_PRESNET_BIT    0

#define LED_GREEN_BLINK     0
#define LED_GREEN           1

#define GET_BIT(data, bit, value)   value = (data >> bit) & 0x1
#define SET_BIT(data, bit)          data |= (1 << bit)
#define CLEAR_BIT(data, bit)        data &= ~(1 << bit)

struct i2c_adap {
	int nr;
	char *name;
	const char *funcs;
	const char *algo;
};

struct i2c_adap *gather_i2c_busses(void);
void free_adapters(struct i2c_adap *adapters);


//#define DEBUG_MSG
#ifdef DEBUG_MSG
    #define debug_print(s) printk s
#else
    #define debug_print(s)
#endif

/* end of compiler conditional */

/* i2c_client Declaration */
static struct i2c_client *x206y_48gt_i2c_client; //0x40 for cpld


/* end of i2c_client Declaration */

/* Function Declaration */
/* i2c-0 */

static ssize_t cpld_byte_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t sfp_status_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t sfp_tx_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t sfp_tx_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t sys_adc_status_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t rst_ctl_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t rst_ctl_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t sys_led_ctrl_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t sys_led_ctrl_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t loc_led_ctrl_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t loc_led_ctrl_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t cpld_ctrl1_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t cpld_ctrl1_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t fan_rpm_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t fan_mode_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t fan_mode_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t fan_stat_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t fan_speed_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t fan_speed_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t fan_board_sel_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t fan_board_sel_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t themal_temp_get(struct device *dev, struct device_attribute *da, char *buf);

static ssize_t psu_stat_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t hw_reset_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t hw_reset_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);


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

/* struct i2c_sysfs_attributes */
enum Asterfusion_i2c_sysfs_attributes
{
    /* i2c-0 */
    /*CPLD 0X30*/
    CPLD_VER,   /*0x00*/
    BOARD_VER,  /*0x01*/
    TEST_REG,   /*0x02*/
    SFP_TX_FAULT,   /*0x03*/
    SFP_PRESENT,    /*0x04*/
    SFP_RX_LOSS,    /*0x05*/
    SFP_TX_STAT,    /*0x06*/
    SFP_RATE_SELECT,/*0x07*/
    ADC_STAT,   /*0x08*/
    RST_CTL,    /*0x09*/
    SYS_LED,    /*0x0A*/
    CPLD_CTL1,  /*0x0B*/
    USB_OC,     /*0x0C*/
    PHY_INT,    /*0x0D*/
    FAN1_OUTLET_RPM,/*0x0E*/
    FAN1_INLET_RPM, /*0x0F*/
    FAN2_OUTLET_RPM,/*0x10*/
    FAN2_INLET_RPM, /*0x11*/
    FAN3_OUTLET_RPM,/*0x12*/
    FAN3_INLET_RPM, /*0x13*/
    FAN_STAT,   /*0x14*/
    FAN_CTL1,   /*0x15*/
    FAN_CTL2,   /*0x16*/
    FAN_BOARD_SEL,  /*0x17*/
    FAN_LM75_R, /*0x18*/
    FAN_LM75_L, /*0x19*/
    AC5X_LM75,  /*0x1A*/
    I2C_ALERT,  /*0x1B*/
    POE_CTL,    /*0x1C*/
    POE_STAT,   /*0x1D*/
    LOC_LED,    /*0x1E*/
    PSU_STAT,   /*0x1F*/
    HW_RESET,    /*0x20*/
};
/* end of struct i2c_sysfs_attributes */

/* sysfs attributes for SENSOR_DEVICE_ATTR */
/* i2c-0 */
/*CPLD 0X30*/


static SENSOR_DEVICE_ATTR(cpld_version      , S_IRUGO           , cpld_byte_get     , NULL              , CPLD_VER);    //0x00
static SENSOR_DEVICE_ATTR(board_version     , S_IRUGO           , cpld_byte_get     , NULL              , BOARD_VER);   //0x01

static SENSOR_DEVICE_ATTR(SFP_tx_fault      , S_IRUGO           , sfp_status_get    , NULL              , SFP_TX_FAULT);//0x03
static SENSOR_DEVICE_ATTR(SFP_present       , S_IRUGO           , sfp_status_get    , NULL              , SFP_PRESENT); //0x04
static SENSOR_DEVICE_ATTR(SFP_rx_loss       , S_IRUGO           , sfp_status_get    , NULL              , SFP_RX_LOSS); //0x05
static SENSOR_DEVICE_ATTR(SFP_tx_stat       , S_IRUGO           , sfp_status_get    , NULL              , SFP_TX_STAT); //0x06
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_54    , S_IRUGO | S_IWUSR , sfp_tx_get        , sfp_tx_set        , SFP54_BIT);//0x06
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_53    , S_IRUGO | S_IWUSR , sfp_tx_get        , sfp_tx_set        , SFP53_BIT);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_52    , S_IRUGO | S_IWUSR , sfp_tx_get        , sfp_tx_set        , SFP52_BIT);//0x06
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_51    , S_IRUGO | S_IWUSR , sfp_tx_get        , sfp_tx_set        , SFP51_BIT);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_50    , S_IRUGO | S_IWUSR , sfp_tx_get        , sfp_tx_set        , SFP50_BIT);
static SENSOR_DEVICE_ATTR(SFP_tx_ctrl_49    , S_IRUGO | S_IWUSR , sfp_tx_get        , sfp_tx_set        , SFP49_BIT);

static SENSOR_DEVICE_ATTR(adc_status        , S_IRUGO           , sys_adc_status_get, NULL              , ADC_STAT);//0x08

static SENSOR_DEVICE_ATTR(pca9548_rst_ctl   , S_IRUGO | S_IWUSR , rst_ctl_get       , rst_ctl_set       , PCA9548_CTL_BIT);//0x09
static SENSOR_DEVICE_ATTR(phy6_rst_ctl      , S_IRUGO | S_IWUSR , rst_ctl_get       , rst_ctl_set       , PHY6_CTL_BIT);
static SENSOR_DEVICE_ATTR(phy5_rst_ctl      , S_IRUGO | S_IWUSR , rst_ctl_get       , rst_ctl_set       , PHY5_CTL_BIT);
static SENSOR_DEVICE_ATTR(phy4_rst_ctl      , S_IRUGO | S_IWUSR , rst_ctl_get       , rst_ctl_set       , PHY4_CTL_BIT);
static SENSOR_DEVICE_ATTR(phy3_rst_ctl      , S_IRUGO | S_IWUSR , rst_ctl_get       , rst_ctl_set       , PHY3_CTL_BIT);
static SENSOR_DEVICE_ATTR(phy2_rst_ctl      , S_IRUGO | S_IWUSR , rst_ctl_get       , rst_ctl_set       , PHY2_CTL_BIT);
static SENSOR_DEVICE_ATTR(phy1_rst_ctl      , S_IRUGO | S_IWUSR , rst_ctl_get       , rst_ctl_set       , PHY1_CTL_BIT);

static SENSOR_DEVICE_ATTR(led_sys           , S_IRUGO | S_IWUSR , sys_led_ctrl_get  , sys_led_ctrl_set  , SYS_LED);//0x0A


static SENSOR_DEVICE_ATTR(eeprom_wp_ctrl    , S_IRUGO | S_IWUSR , cpld_ctrl1_get    , cpld_ctrl1_set    , EEPROM_WP_CTRL_BIT);//0x0B
static SENSOR_DEVICE_ATTR(over_temp_ctrl    , S_IRUGO | S_IWUSR , cpld_ctrl1_get    , cpld_ctrl1_set    , OVER_TEMP_CTRL_BIT);
static SENSOR_DEVICE_ATTR(wdt_ctrl          , S_IRUGO | S_IWUSR , cpld_ctrl1_get    , cpld_ctrl1_set    , WDT_CTRL_BIT);

static SENSOR_DEVICE_ATTR(fan1_outlet_rpm   , S_IRUGO           , fan_rpm_get       , NULL              , FAN1_OUTLET_RPM);//0x0E
static SENSOR_DEVICE_ATTR(fan1_inlet_rpm    , S_IRUGO           , fan_rpm_get       , NULL              , FAN1_INLET_RPM);//0x0F
static SENSOR_DEVICE_ATTR(fan2_outlet_rpm   , S_IRUGO           , fan_rpm_get       , NULL              , FAN2_OUTLET_RPM);//0x10
static SENSOR_DEVICE_ATTR(fan2_inlet_rpm    , S_IRUGO           , fan_rpm_get       , NULL              , FAN2_INLET_RPM);//0x011
static SENSOR_DEVICE_ATTR(fan3_outlet_rpm   , S_IRUGO           , fan_rpm_get       , NULL              , FAN3_OUTLET_RPM);//0x12
static SENSOR_DEVICE_ATTR(fan3_inlet_rpm    , S_IRUGO           , fan_rpm_get       , NULL              , FAN3_INLET_RPM);//0x13

static SENSOR_DEVICE_ATTR(fan1_status       , S_IRUGO           , fan_stat_get      , NULL              , FAN1_STATUS_BIT);//0x14
static SENSOR_DEVICE_ATTR(fan2_status       , S_IRUGO           , fan_stat_get      , NULL              , FAN2_STATUS_BIT);
static SENSOR_DEVICE_ATTR(fan3_status       , S_IRUGO           , fan_stat_get      , NULL              , FAN3_STATUS_BIT);
static SENSOR_DEVICE_ATTR(fan1_present      , S_IRUGO           , fan_stat_get      , NULL              , FAN1_PRESNET_BIT);
static SENSOR_DEVICE_ATTR(fan2_present      , S_IRUGO           , fan_stat_get      , NULL              , FAN2_PRESNET_BIT);
static SENSOR_DEVICE_ATTR(fan3_present      , S_IRUGO           , fan_stat_get      , NULL              , FAN3_PRESNET_BIT);

static SENSOR_DEVICE_ATTR(fan_mode          , S_IRUGO | S_IWUSR , fan_mode_get      , fan_mode_set      , FAN_CTL1);//0x15
static SENSOR_DEVICE_ATTR(fan_speed         , S_IRUGO | S_IWUSR , fan_speed_get     , fan_speed_set     , FAN_CTL2);//0x16
static SENSOR_DEVICE_ATTR(fan_board_sel     , S_IRUGO | S_IWUSR , fan_board_sel_get , fan_board_sel_set , FAN_BOARD_SEL);//0x17

static SENSOR_DEVICE_ATTR(fan_lm75_right    , S_IRUGO           , themal_temp_get   , NULL              , FAN_LM75_R);//0x18
static SENSOR_DEVICE_ATTR(fan_lm75_left     , S_IRUGO           , themal_temp_get   , NULL              , FAN_LM75_L);//0x19
static SENSOR_DEVICE_ATTR(ac5x_lm75         , S_IRUGO           , themal_temp_get   , NULL              , AC5X_LM75);//0x1A

static SENSOR_DEVICE_ATTR(led_loc           , S_IRUGO | S_IWUSR , loc_led_ctrl_get  , loc_led_ctrl_set  , LOC_LED);//0x1E

static SENSOR_DEVICE_ATTR(psu1_present      , S_IRUGO           , psu_stat_get       , NULL              , PSU1_PRESNET_BIT);//0x1F
static SENSOR_DEVICE_ATTR(psu2_present      , S_IRUGO           , psu_stat_get       , NULL              , PSU2_PRESNET_BIT);
static SENSOR_DEVICE_ATTR(psu1_status       , S_IRUGO           , psu_stat_get       , NULL              , PSU1_ALERT_BIT);
static SENSOR_DEVICE_ATTR(psu2_status       , S_IRUGO           , psu_stat_get       , NULL              , PSU2_ALERT_BIT);
static SENSOR_DEVICE_ATTR(psu1_power        , S_IRUGO           , psu_stat_get       , NULL              , PSU1_POWER_BIT);
static SENSOR_DEVICE_ATTR(psu2_power        , S_IRUGO           , psu_stat_get       , NULL              , PSU2_POWER_BIT);

static SENSOR_DEVICE_ATTR(hw_reset          , S_IRUGO           , hw_reset_get       , hw_reset_set      , HW_RESET);



/* end of sysfs attributes for SENSOR_DEVICE_ATTR */

/* sysfs attributes for hwmon */
/* i2c-0 */
static struct attribute *X206Y_48GT_SYS_attributes[] =
{
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    &sensor_dev_attr_board_version.dev_attr.attr,
    &sensor_dev_attr_adc_status.dev_attr.attr,
    &sensor_dev_attr_eeprom_wp_ctrl.dev_attr.attr,
    &sensor_dev_attr_over_temp_ctrl.dev_attr.attr,
    &sensor_dev_attr_wdt_ctrl.dev_attr.attr,
    NULL
};

static struct attribute *X206Y_48GT_SFP_attributes[] =
{
    &sensor_dev_attr_SFP_tx_fault.dev_attr.attr,
    &sensor_dev_attr_SFP_present.dev_attr.attr,
    &sensor_dev_attr_SFP_rx_loss.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_stat.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_49.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_50.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_51.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_52.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_53.dev_attr.attr,
    &sensor_dev_attr_SFP_tx_ctrl_54.dev_attr.attr,
    NULL
};

static struct attribute *X206Y_48GT_Reset_attributes[] =
{
    &sensor_dev_attr_pca9548_rst_ctl.dev_attr.attr,
    &sensor_dev_attr_phy6_rst_ctl.dev_attr.attr,
    &sensor_dev_attr_phy5_rst_ctl.dev_attr.attr,
    &sensor_dev_attr_phy4_rst_ctl.dev_attr.attr,
    &sensor_dev_attr_phy3_rst_ctl.dev_attr.attr,
    &sensor_dev_attr_phy2_rst_ctl.dev_attr.attr,
    &sensor_dev_attr_phy1_rst_ctl.dev_attr.attr,
    &sensor_dev_attr_hw_reset.dev_attr.attr,
    NULL
};

static struct attribute *X206Y_48GT_FAN_attributes[] = {
    &sensor_dev_attr_fan1_outlet_rpm.dev_attr.attr,
    &sensor_dev_attr_fan1_inlet_rpm.dev_attr.attr,
    &sensor_dev_attr_fan2_outlet_rpm.dev_attr.attr,
    &sensor_dev_attr_fan2_inlet_rpm.dev_attr.attr,
    &sensor_dev_attr_fan3_outlet_rpm.dev_attr.attr,
    &sensor_dev_attr_fan3_inlet_rpm.dev_attr.attr,
    &sensor_dev_attr_fan1_status.dev_attr.attr,
    &sensor_dev_attr_fan2_status.dev_attr.attr,
    &sensor_dev_attr_fan3_status.dev_attr.attr,
    &sensor_dev_attr_fan1_present.dev_attr.attr,
    &sensor_dev_attr_fan2_present.dev_attr.attr,
    &sensor_dev_attr_fan3_present.dev_attr.attr,
    &sensor_dev_attr_fan_mode.dev_attr.attr,
    &sensor_dev_attr_fan_speed.dev_attr.attr,
    &sensor_dev_attr_fan_board_sel.dev_attr.attr,
    NULL
};

static struct attribute *X206Y_48GT_Sensor_attributes[] =
{
    &sensor_dev_attr_fan_lm75_right.dev_attr.attr,
    &sensor_dev_attr_fan_lm75_left.dev_attr.attr,
    &sensor_dev_attr_ac5x_lm75.dev_attr.attr,
    NULL
};

static struct attribute *X206Y_48GT_Led_attributes[] =
{
    &sensor_dev_attr_led_sys.dev_attr.attr,
    &sensor_dev_attr_led_loc.dev_attr.attr,
    NULL
};

static struct attribute *X206Y_48GT_PSU_attributes[] =
{
    &sensor_dev_attr_psu1_present.dev_attr.attr,
    &sensor_dev_attr_psu2_present.dev_attr.attr,
    &sensor_dev_attr_psu1_status.dev_attr.attr,
    &sensor_dev_attr_psu2_status.dev_attr.attr,
    &sensor_dev_attr_psu1_power.dev_attr.attr,
    &sensor_dev_attr_psu2_power.dev_attr.attr,
    NULL
};

/* end of sysfs attributes for hwmon */

/* struct attribute_group */
static const struct attribute_group X206Y_48GT_SYS_group =
{
    .name  = "X206Y_48GT_SYS",
    .attrs = X206Y_48GT_SYS_attributes,
};

static const struct attribute_group X206Y_48GT_Reset_group =
{
    .name  = "X206Y_48GT_Reset",
    .attrs = X206Y_48GT_Reset_attributes,
};

static const struct attribute_group X206Y_48GT_Sensor_group =
{
    .name  = "X206Y_48GT_Sensor",
    .attrs = X206Y_48GT_Sensor_attributes,
};

static const struct attribute_group X206Y_48GT_Led_group =
{
    .name  = "X206Y_48GT_Led",
    .attrs = X206Y_48GT_Led_attributes,
};

static const struct attribute_group X206Y_48GT_SFP_group =
{
    .name  = "X206Y_48GT_SFP",
    .attrs = X206Y_48GT_SFP_attributes,
};


static const struct attribute_group X206Y_48GT_FAN_group =
{
    .name  = "X206Y_48GT_FAN",
    .attrs = X206Y_48GT_FAN_attributes,
};

static const struct attribute_group X206Y_48GT_PSU_group =
{
    .name  = "X206Y_48GT_PSU",
    .attrs = X206Y_48GT_PSU_attributes,
};

/* end of struct attribute_group */
