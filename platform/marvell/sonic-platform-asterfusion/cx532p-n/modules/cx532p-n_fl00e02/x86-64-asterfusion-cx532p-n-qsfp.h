/* register offset define */
#define QSFP_RESET_REG_0        0xc
#define QSFP_RESET_REG_1        0xd
#define QSFP_RESET_REG_2        0xe
#define QSFP_RESET_REG_3        0xf
#define QSFP_PRESENT_REG_0      0x6
#define QSFP_PRESENT_REG_1      0x7
#define QSFP_PRESENT_REG_2      0x8
#define QSFP_PRESENT_REG_3      0x9
#define QSFP_PRESENT_REG_AUX    0x10             /*aux X1, X2*/
#define QSFP_TXDIS_REG_AUX      0x10            /*aux X1, X2*/
#define QSFP_RESET              1
#define XPORT_LED_MODE_REG      0x1a             /*X1, X2 led mode ctrl*/

unsigned char qsfp_present_regs[35][2] = {
    {0x00, 0x00},  //cpld offset, bit mask
    {QSFP_PRESENT_REG_0, 0x01},
    {QSFP_PRESENT_REG_0, 0x02},
    {QSFP_PRESENT_REG_0, 0x04},
    {QSFP_PRESENT_REG_0, 0x08},
    {QSFP_PRESENT_REG_0, 0x10},
    {QSFP_PRESENT_REG_0, 0x20},
    {QSFP_PRESENT_REG_0, 0x40},
    {QSFP_PRESENT_REG_0, 0x80},
    {QSFP_PRESENT_REG_1, 0x01},
    {QSFP_PRESENT_REG_1, 0x02},
    {QSFP_PRESENT_REG_1, 0x04},
    {QSFP_PRESENT_REG_1, 0x08},
    {QSFP_PRESENT_REG_1, 0x10},
    {QSFP_PRESENT_REG_1, 0x20},
    {QSFP_PRESENT_REG_1, 0x40},
    {QSFP_PRESENT_REG_1, 0x80},
    {QSFP_PRESENT_REG_2, 0x01},
    {QSFP_PRESENT_REG_2, 0x02},
    {QSFP_PRESENT_REG_2, 0x04},
    {QSFP_PRESENT_REG_2, 0x08},
    {QSFP_PRESENT_REG_2, 0x10},
    {QSFP_PRESENT_REG_2, 0x20},
    {QSFP_PRESENT_REG_2, 0x40},
    {QSFP_PRESENT_REG_2, 0x80},
    {QSFP_PRESENT_REG_3, 0x01},
    {QSFP_PRESENT_REG_3, 0x02},
    {QSFP_PRESENT_REG_3, 0x04},
    {QSFP_PRESENT_REG_3, 0x08},
    {QSFP_PRESENT_REG_3, 0x10},
    {QSFP_PRESENT_REG_3, 0x20},
    {QSFP_PRESENT_REG_3, 0x40},
    {QSFP_PRESENT_REG_3, 0x80},
    {QSFP_PRESENT_REG_AUX, 0x1},             /*AUX X1*/
    {QSFP_PRESENT_REG_AUX, 0x8}              /*AUX X2*/
};

unsigned char qsfp_reset_regs[33][2] = {
    {0x00, 0x00},  //cpld offset, bit mask
    {QSFP_RESET_REG_0, 0x01},
    {QSFP_RESET_REG_0, 0x02},
    {QSFP_RESET_REG_0, 0x04},
    {QSFP_RESET_REG_0, 0x08},
    {QSFP_RESET_REG_0, 0x10},
    {QSFP_RESET_REG_0, 0x20},
    {QSFP_RESET_REG_0, 0x40},
    {QSFP_RESET_REG_0, 0x80},
    {QSFP_RESET_REG_1, 0x01},
    {QSFP_RESET_REG_1, 0x02},
    {QSFP_RESET_REG_1, 0x04},
    {QSFP_RESET_REG_1, 0x08},
    {QSFP_RESET_REG_1, 0x10},
    {QSFP_RESET_REG_1, 0x20},
    {QSFP_RESET_REG_1, 0x40},
    {QSFP_RESET_REG_1, 0x80},
    {QSFP_RESET_REG_2, 0x01},
    {QSFP_RESET_REG_2, 0x02},
    {QSFP_RESET_REG_2, 0x04},
    {QSFP_RESET_REG_2, 0x08},
    {QSFP_RESET_REG_2, 0x10},
    {QSFP_RESET_REG_2, 0x20},
    {QSFP_RESET_REG_2, 0x40},
    {QSFP_RESET_REG_2, 0x80},
    {QSFP_RESET_REG_3, 0x01},
    {QSFP_RESET_REG_3, 0x02},
    {QSFP_RESET_REG_3, 0x04},
    {QSFP_RESET_REG_3, 0x08},
    {QSFP_RESET_REG_3, 0x10},
    {QSFP_RESET_REG_3, 0x20},
    {QSFP_RESET_REG_3, 0x40},
    {QSFP_RESET_REG_3, 0x80}
};
/* end of register offset define */
