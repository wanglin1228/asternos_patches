/*
 * asterfusion_x20xp_xxs_sfp.c - A driver to read and write the EEPROM/STATUS on optical transceiver
 *
 **/

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

#define MAX_PORT_NUM                18
#define MAX_10G_PORT_NUM            16
#define MAX_100G_PORT_NUM           2


#define GET_BIT(data, bit, value)   value = (data >> bit) & 0x1
#define SET_BIT(data, bit)          data |= (1 << bit)
#define CLEAR_BIT(data, bit)        data &= ~(1 << bit)

enum sfp_croups { sfp_group};

struct x20xp_xxs_sfp_platform_data {
    u32     byte_len;       /* size (sum of all addr) */
    u16     page_size;      /* for writes */
    u8      flags;
    void    *dummy1;        /* backward compatibility */
    void    *dummy2;        /* backward compatibility */

    /* dev_class: ONE_ADDR (QSFP) or TWO_ADDR (SFP) */
    int dev_class;
    unsigned int write_max;
};

struct x20xp_xxs_sfp_data {
    struct x20xp_xxs_sfp_platform_data chip[MAX_PORT_NUM];

    /*
     * Lock protects against activities from other Linux tasks,
     * but not from changes by other I2C masters.
     */
    struct mutex lock;
    struct bin_attribute bin;
    kernel_ulong_t driver_data;

    struct i2c_client *client[];
};

/*
 * This parameter is to help this driver avoid blocking other drivers out
 * of I2C for potentially troublesome amounts of time. With a 100 kHz I2C
 * clock, one 256 byte read takes about 1/43 second which is excessive;
 * but the 1/170 second it takes at 400 kHz may be quite reasonable; and
 * at 1 MHz (Fm+) a 1/430 second delay could easily be invisible.
 *
 * This value is forced to be a power of two so that writes align on pages.
 */
static unsigned int io_limit = SFP_PAGE_SIZE;

extern int asterfusion_x20xp_cpld_read(unsigned short cpld_addr, u8 reg);
extern int asterfusion_x20xp_cpld_write(unsigned short cpld_addr, u8 reg, u8 value);
extern int asterfusion_x20xp_cpld_reset(void);
extern void asterfusion_x20xp_read_lock(void);
extern void asterfusion_x20xp_read_unlock(void);

/*
 * specs often allow 5 msec for a page write, sometimes 20 msec;
 * it's important to recover from write timeouts.
 */
static unsigned int write_timeout = 25;

static char SFP_EEPROM_MAPPING[MAX_PORT_NUM][16]={0};


/*-------------------------------------------------------------------------*/
/*
 * This routine computes the addressing information to be used for
 * a given r/w request.
 *
 * Task is to calculate the client (0 = i2c addr 50, 1 = i2c addr 51),
 * the page, and the offset.
 *
 * Handles both single address (eg QSFP) and two address (eg SFP).
 *     For SFP, offset 0-255 are on client[0], >255 is on client[1]
 *     Offset 256-383 are on the lower half of client[1]
 *     Pages are accessible on the upper half of client[1].
 *     Offset >383 are in 128 byte pages mapped into the upper half
 *
 *     For QSFP, all offsets are on client[0]
 *     offset 0-127 are on the lower half of client[0] (no paging)
 *     Pages are accessible on the upper half of client[1].
 *     Offset >127 are in 128 byte pages mapped into the upper half
 *
 *     Callers must not read/write beyond the end of a client or a page
 *     without recomputing the client/page.  Hence offset (within page)
 *     plus length must be less than or equal to 128.  (Note that this
 *     routine does not have access to the length of the call, hence
 *     cannot do the validity check.)
 *
 * Offset within Lower Page 00h and Upper Page 00h are not recomputed
 */

static uint8_t x20xp_xxs_sfp_translate_offset(struct x20xp_xxs_sfp_data *sfp,
        loff_t *offset, struct i2c_client **client, int num)
{
    unsigned int page = 0;
    *client = sfp->client[0];

    /* if SFP style, offset > 255, shift to i2c addr 0x51 */
    if (sfp->chip[num].dev_class == TWO_ADDR) {
        if (*offset > 255) {
            /* like QSFP, but shifted to client[1] */
            *client = sfp->client[1];
            *offset -= 256;
        }
    }

    /*
     * if offset is in the range 0-128...
     * page doesn't matter (using lower half), return 0.
     * offset is already correct (don't add 128 to get to paged area)
     */
    if (*offset < SFP_PAGE_SIZE)
        return page;

    /* note, page will always be positive since *offset >= 128 */
    page = (*offset >> 7)-1;
    /* 0x80 places the offset in the top half, offset is last 7 bits */
    *offset = SFP_PAGE_SIZE + (*offset & 0x7f);

    return page;  /* note also returning clix20xp_xxs_sfp_translate_offsetent and offset */
}

static ssize_t x20xp_xxs_sfp_eeprom_read(struct x20xp_xxs_sfp_data *sfp,
            struct i2c_client *client,
            char *buf, unsigned int offset, size_t count)
{
    unsigned long timeout, read_time;
    int status;

    /*smaller eeproms can work given some SMBus extension calls */
    if (count > I2C_SMBUS_BLOCK_MAX)
        count = I2C_SMBUS_BLOCK_MAX;
    /*
     * Reads fail if the previous write didn't complete yet. We may
     * loop a few times until this one succeeds, waiting at least
     * long enough for one entire page write to work.
     */
    timeout = jiffies + msecs_to_jiffies(write_timeout);
    do {

        read_time = jiffies;
        status = i2c_smbus_read_i2c_block_data(client, offset, count, buf);

        dev_dbg(&client->dev, "eeprom read %zu@%d --> %d (%ld)\n",
                count, offset, status, jiffies);

        if (status == count)  /* happy path */
            return count;

        if (status == -ENXIO) /* no module present */
            return status;

        /* REVISIT: at HZ=100, this is sloooow */
        usleep_range(1000, 2000);
    } while (time_before(read_time, timeout));

    return -ETIMEDOUT;
}

static ssize_t x20xp_xxs_sfp_eeprom_write(struct x20xp_xxs_sfp_data *sfp,
                struct i2c_client *client,
                const char *buf,
                unsigned int offset, size_t count)
{
    ssize_t status;
    unsigned long timeout, write_time;
    unsigned int next_page_start;
    int i = 0;

    /* write max is at most a page
     * (In this driver, write_max is actually one byte!)
     */
    if (count > sfp->chip[i].write_max)
        count = sfp->chip[i].write_max;

    /* shorten count if necessary to avoid crossing page boundary */
    next_page_start = roundup(offset + 1, SFP_PAGE_SIZE);
    if (offset + count > next_page_start)
        count = next_page_start - offset;

    if (count > I2C_SMBUS_BLOCK_MAX)
        count = I2C_SMBUS_BLOCK_MAX;

    /*
     * Reads fail if the previous write didn't complete yet. We may
     * loop a few times until this one succeeds, waiting at least
     * long enough for one entire page write to work.
     */
    timeout = jiffies + msecs_to_jiffies(write_timeout);
    do {
        write_time = jiffies;

        status = i2c_smbus_write_i2c_block_data(client,
                    offset, count, buf);
        if (status == 0)
            return count;

        /* REVISIT: at HZ=100, this is sloooow */
        usleep_range(1000, 2000);
    } while (time_before(write_time, timeout));

    return -ETIMEDOUT;
}

static ssize_t x20xp_xxs_sfp_eeprom_update_client(struct x20xp_xxs_sfp_data *sfp,
                char *buf, loff_t off, size_t count, int num)
{
    struct i2c_client *client;
    ssize_t retval = 0;
    uint8_t page = 0;
    loff_t phy_offset = off;
    int ret = 0;

    page = x20xp_xxs_sfp_translate_offset(sfp, &phy_offset, &client, num);

    dev_dbg(&client->dev,
        "%s off %lld  page:%d phy_offset:%lld, count:%ld\n",
        __func__, off, page, phy_offset, (long int) count);
    if (page > 0) {
        ret = x20xp_xxs_sfp_eeprom_write(sfp, client, &page,
            SFP_PAGE_SELECT_REG, 1);
        if (ret < 0) {
            dev_dbg(&client->dev, "Write page register for page %d failed ret:%d!\n", page, ret);
            return ret;
        }
    }

    while (count) {
        ssize_t    status;

        status =  x20xp_xxs_sfp_eeprom_read(sfp, client,
            buf, phy_offset, count);

        if (status <= 0) {
            if (retval == 0)
                retval = status;
            break;
        }
        buf += status;
        phy_offset += status;
        count -= status;
        retval += status;
    }

    if (page > 0) {
        /* return the page register to page 0 (why?) */
        page = 0;
        ret = x20xp_xxs_sfp_eeprom_write(sfp, client, &page,
            SFP_PAGE_SELECT_REG, 1);
        if (ret < 0) {
            dev_err(&client->dev,"Restore page register to 0 failed:%d!\n", ret);
            /* error only if nothing has been transferred */
            if (retval == 0)
                retval = ret;
        }
    }
    return retval;
}
/*
 * Figure out if this access is within the range of supported pages.
 * Note this is called on every access because we don't know if the
 * module has been replaced since the last call.
 * If/when modules support more pages, this is the routine to update
 * to validate and allow access to additional pages.
 *
 * Returns updated len for this access:
 *     - entire access is legal, original len is returned.
 *     - access begins legal but is too long, len is truncated to fit.
 *     - initial offset exceeds supported pages, return OPTOE_EOF (zero)
 */
static ssize_t x20xp_xxs_sfp_page_legal(struct x20xp_xxs_sfp_data *sfp,
        loff_t off, size_t len, int num)
{
    struct i2c_client *client = sfp->client[0];
    u8 regval;
    int not_pageable;
    int status;
    size_t maxlen;

    if (off < 0)
        return -EINVAL;

    if (sfp->chip[num].dev_class == TWO_ADDR) {
        /* SFP case */
        /* if only using addr 0x50 (first 256 bytes) we're good */
        if ((off + len) <= TWO_ADDR_NO_0X51_SIZE)
            return len;
        /* if offset exceeds possible pages, we're not good */
        if (off >= TWO_ADDR_EEPROM_SIZE)
            return SFP_EOF;
        /* in between, are pages supported? */

        status = x20xp_xxs_sfp_eeprom_read(sfp, client, &regval,
                TWO_ADDR_PAGEABLE_REG, 1);

        if (status < 0)
            return status;  /* error out (no module?) */
        if (regval & TWO_ADDR_PAGEABLE) {
            /* Pages supported, trim len to the end of pages */
            maxlen = TWO_ADDR_EEPROM_SIZE - off;
        } else {
            /* pages not supported, trim len to unpaged size */
            if (off >= TWO_ADDR_EEPROM_UNPAGED_SIZE)
                return SFP_EOF;

            /* will be accessing addr 0x51, is that supported? */
            /* byte 92, bit 6 implies DDM support, 0x51 support */

            status = x20xp_xxs_sfp_eeprom_read(sfp, client, &regval,
                        TWO_ADDR_0X51_REG, 1);

            if (status < 0)
                return status;
            if (regval & TWO_ADDR_0X51_SUPP) {
                /* addr 0x51 is OK */
                maxlen = TWO_ADDR_EEPROM_UNPAGED_SIZE - off;
            } else {
                /* addr 0x51 NOT supported, trim to 256 max */
                if (off >= TWO_ADDR_NO_0X51_SIZE)
                    return SFP_EOF;
                maxlen = TWO_ADDR_NO_0X51_SIZE - off;
            }
        }
        len = (len > maxlen) ? maxlen : len;
        dev_dbg(&client->dev,
            "page_legal, SFP, off %lld len %ld\n",
            off, (long int) len);
    } else {
        /* QSFP case, CMIS case */
        /* if no pages needed, we're good */
        if ((off + len) <= ONE_ADDR_EEPROM_UNPAGED_SIZE)
            return len;
        /* if offset exceeds possible pages, we're not good */
        if (off >= ONE_ADDR_EEPROM_SIZE)
            return SFP_EOF;
        /* in between, are pages supported? */

        status = x20xp_xxs_sfp_eeprom_read(sfp, client, &regval,
                ONE_ADDR_PAGEABLE_REG, 1);

        if (status < 0)
            return status;  /* error out (no module?) */

        if (sfp->chip[num].dev_class == ONE_ADDR) {
            not_pageable = QSFP_NOT_PAGEABLE;
        } else {
            not_pageable = CMIS_NOT_PAGEABLE;
        }
        dev_dbg(&client->dev,
            "Paging Register: 0x%x; not_pageable mask: 0x%x\n",
            regval, not_pageable);

        if (regval & not_pageable) {
            /* pages not supported, trim len to unpaged size */
            if (off >= ONE_ADDR_EEPROM_UNPAGED_SIZE)
                return SFP_EOF;
            maxlen = ONE_ADDR_EEPROM_UNPAGED_SIZE - off;
        } else {
            /* Pages supported, trim len to the end of pages */
            maxlen = ONE_ADDR_EEPROM_SIZE - off;
        }
        len = (len > maxlen) ? maxlen : len;
        dev_dbg(&client->dev,
            "page_legal, QSFP, off %lld len %ld\n",
            off, (long int) len);
    }
    return len;
}

static ssize_t x20xp_xxs_sfp_read(struct x20xp_xxs_sfp_data *sfp,
        char *buf, loff_t off, size_t len, int num)
{
    int chunk;
    int status = 0;
    ssize_t retval;
    size_t pending_len = 0, chunk_len = 0;
    loff_t chunk_offset = 0, chunk_start_offset = 0;
    loff_t chunk_end_offset = 0;

    if (unlikely(!len))
        return len;
    /*
     * Read data from chip, protecting against concurrent updates
     * from this host, but not from other I2C masters.
     */
    mutex_lock(&sfp->lock);
    DBG(printk(KERN_ALERT "############ %s - mutex_lock(&sfp->lock) for interface %d!\n", __func__, num));
    /*
     * Confirm this access fits within the device suppored addr range
     */
    status = x20xp_xxs_sfp_page_legal(sfp, off, len, num);
    if ((status == SFP_EOF) || (status < 0)) {
        DBG(printk(KERN_ALERT "############ %s - x20xp_xxs_sfp_page_legal() for interface %d!\n", __func__, num));
        mutex_unlock(&sfp->lock);
        DBG(printk(KERN_ALERT "############ %s - 111 mutex_unlock(&sfp->lock) for interface %d!\n", __func__, num));
        return status;
    }
    len = status;

    /*
     * For each (128 byte) chunk involved in this request, issue a
     * separate call to sff_eeprom_update_client(), to
     * ensure that each access recalculates the client/page
     * and writes the page register as needed.
     * Note that chunk to page mapping is confusing, is different for
     * QSFP and SFP, and never needs to be done.  Don't try!
     */
    pending_len = len; /* amount remaining to transfer */
    retval = 0;  /* amount transferred */
    for (chunk = off >> 7; chunk <= (off + len - 1) >> 7; chunk++) {

        /*
         * Compute the offset and number of bytes to be read/write
         *
         * 1. start at an offset not equal to 0 (within the chunk)
         *    and read/write less than the rest of the chunk
         * 2. start at an offset not equal to 0 and read/write the rest
         *    of the chunk
         * 3. start at offset 0 (within the chunk) and read/write less
         *    than entire chunk
         * 4. start at offset 0 (within the chunk), and read/write
         *    the entire chunk
         */
        chunk_start_offset = chunk * SFP_PAGE_SIZE;
        chunk_end_offset = chunk_start_offset + SFP_PAGE_SIZE;

        if (chunk_start_offset < off) {
            chunk_offset = off;
            if ((off + pending_len) < chunk_end_offset)
                chunk_len = pending_len;
            else
                chunk_len = chunk_end_offset - off;
        } else {
            chunk_offset = chunk_start_offset;
            if (pending_len < SFP_PAGE_SIZE)
                chunk_len = pending_len;
            else
                chunk_len = SFP_PAGE_SIZE;
        }

        /*
         * note: chunk_offset is from the start of the EEPROM,
         * not the start of the chunk
         */
        status = x20xp_xxs_sfp_eeprom_update_client(sfp, buf,
                chunk_offset, chunk_len, num);

        if (status != chunk_len) {
            /* This is another 'no device present' path */
            if (status > 0)
                retval += status;
            if (retval == 0)
                retval = status;
            break;
        }
        buf += status;
        pending_len -= status;
        retval += status;
    }
    mutex_unlock(&sfp->lock);
    DBG(printk(KERN_ALERT "############ %s - 2222 mutex_unlock(&sfp->lock) for interface %d!\n", __func__, num));

    return retval;
}

static ssize_t x20xp_xxs_sfp_bin_read(struct file *filp, struct kobject *kobj,
        struct bin_attribute *attr,
        char *buf, loff_t off, size_t count)
{
    int i;
    int pca9548Reg;
    struct x20xp_xxs_sfp_data *sfp;
    ssize_t retval = 0;

    sfp = dev_get_drvdata(container_of(kobj, struct device, kobj));

    for(i = 0; i < MAX_PORT_NUM; i++)
    {
        if(!strcmp(attr->attr.name, SFP_EEPROM_MAPPING[i]))
        {
            asterfusion_x20xp_read_lock();
            DBG(printk(KERN_ALERT "############ %s - asterfusion_x20xp_read_lock for interface %d!\n", __func__, i));

            asterfusion_x20xp_cpld_reset();
            if (i < MAX_10G_PORT_NUM)
            {
                pca9548Reg = i/8 + PCA9548_0X73;
                asterfusion_x20xp_cpld_write(pca9548Reg, SFP_SCL_BASE, 1<<(i%8));
            }
            else
            {
                pca9548Reg = PCA9548_0X76;
                asterfusion_x20xp_cpld_write(pca9548Reg, SFP_SCL_BASE, 1<<(i%8 + 4));
            }

            retval = x20xp_xxs_sfp_read(sfp, buf, off, count, i);

            asterfusion_x20xp_read_unlock();
            DBG(printk(KERN_ALERT "############ %s - asterfusion_x20xp_read_unlock for interface %d!\n", __func__, i));
            break;
        }
    }

    return retval;
}

#define SFP_EEPROM_ATTR(_num)    \
        static struct bin_attribute sfp##_num##_eeprom_attr = {   \
                .attr = {   \
                        .name =  __stringify(sfp##_num##_eeprom),  \
                        .mode = S_IRUGO\
                },  \
                .size = TWO_ADDR_EEPROM_SIZE,    \
                .read = x20xp_xxs_sfp_bin_read,   \
                }

#define QSFP_EEPROM_ATTR(_num)    \
        static struct bin_attribute qsfp##_num##_eeprom_attr = {   \
                .attr = {   \
                        .name =  __stringify(sfp##_num##_eeprom),  \
                        .mode = S_IRUGO\
                },  \
                .size = ONE_ADDR_EEPROM_SIZE,    \
                .read = x20xp_xxs_sfp_bin_read,   \
                }

SFP_EEPROM_ATTR(1);SFP_EEPROM_ATTR(2);SFP_EEPROM_ATTR(3);SFP_EEPROM_ATTR(4);SFP_EEPROM_ATTR(5);SFP_EEPROM_ATTR(6);SFP_EEPROM_ATTR(7);SFP_EEPROM_ATTR(8);
SFP_EEPROM_ATTR(9);SFP_EEPROM_ATTR(10);SFP_EEPROM_ATTR(11);SFP_EEPROM_ATTR(12);SFP_EEPROM_ATTR(13);SFP_EEPROM_ATTR(14);SFP_EEPROM_ATTR(15);SFP_EEPROM_ATTR(16);
QSFP_EEPROM_ATTR(17);QSFP_EEPROM_ATTR(18);



static struct bin_attribute *x20xp_xxs_sfp_eeprom_attributes[] = {
    &sfp1_eeprom_attr, &sfp2_eeprom_attr, &sfp3_eeprom_attr, &sfp4_eeprom_attr, &sfp5_eeprom_attr, &sfp6_eeprom_attr, &sfp7_eeprom_attr, &sfp8_eeprom_attr,
    &sfp9_eeprom_attr, &sfp10_eeprom_attr, &sfp11_eeprom_attr, &sfp12_eeprom_attr, &sfp13_eeprom_attr, &sfp14_eeprom_attr, &sfp15_eeprom_attr, &sfp16_eeprom_attr,
    &qsfp17_eeprom_attr, &qsfp18_eeprom_attr,
    NULL
};

static const struct attribute_group x20xp_xxs_sfp_eeprom_group = { .bin_attrs = x20xp_xxs_sfp_eeprom_attributes};

static int x20xp_xxs_sfp_device_probe(struct i2c_client *client, const struct i2c_device_id *dev_id)
{
    int i, err;
    struct x20xp_xxs_sfp_platform_data chip[MAX_PORT_NUM];
    struct x20xp_xxs_sfp_data *sfp;
    unsigned int write_max = 1;

    if (client->addr != SFP_EEPROM_A0_ADDR) {
        DBG(printk(KERN_ALERT "%s - probe, bad i2c addr: 0x%x\n", __func__, client->addr));
        err = -EINVAL;
        goto exit;
    }

    sfp = kzalloc(sizeof(struct x20xp_xxs_sfp_data) +
            NUM_ADDRESS * sizeof(struct i2c_client *),
            GFP_KERNEL);
    if (!sfp) {
        err = -ENOMEM;
        goto exit;
    }

    mutex_init(&sfp->lock);

    err = sysfs_create_group(&client->dev.kobj, &x20xp_xxs_sfp_eeprom_group);
    if (err)
        goto err_clients;
    
    if (write_max > io_limit)
        write_max = io_limit;

    if (write_max > I2C_SMBUS_BLOCK_MAX)
        write_max = I2C_SMBUS_BLOCK_MAX;

    chip[i].write_max = write_max;

    for(i=0; i<MAX_PORT_NUM; i++)
    {
        if (i > (MAX_10G_PORT_NUM - 1)) {
            chip[i].dev_class = ONE_ADDR;
            chip[i].byte_len = ONE_ADDR_EEPROM_SIZE;
        }
        else
        {
            chip[i].dev_class = TWO_ADDR;
            chip[i].byte_len = TWO_ADDR_EEPROM_SIZE;
        }
        sfp->chip[i] = chip[i];
    }

    sfp->driver_data = dev_id->driver_data;
    sfp->client[0] = client;

    /* SFF-8472 spec requires that the second I2C address be 0x51 */
    if (NUM_ADDRESS == 2) {
        sfp->client[1] = i2c_new_dummy_device(client->adapter, SFP_EEPROM_A2_ADDR);
        if (!sfp->client[1]) {
            printk(KERN_ALERT "%s - address 0x51 unavailable\n", __func__);
            err = -EADDRINUSE;
            goto err_struct;
        }
    }

    i2c_set_clientdata(client, sfp);

    return 0;

err_struct:
    if (NUM_ADDRESS == 2) {
        if (sfp->client[1])
            i2c_unregister_device(sfp->client[1]);
    }
err_clients:
    kfree(sfp);
exit:
    DBG(printk(KERN_ALERT "%s - probe error %d\n", __func__, err));
    return err;
}

static int x20xp_xxs_sfp_device_remove(struct i2c_client *client)
{
    struct x20xp_xxs_sfp_data *sfp;
    int i;

    sfp = i2c_get_clientdata(client);

    sysfs_remove_group(&client->dev.kobj, &x20xp_xxs_sfp_eeprom_group);

    for (i = 1; i < NUM_ADDRESS; i++)
        i2c_unregister_device(sfp->client[i]);

    kfree(sfp);

    return 0;
}

static const struct i2c_device_id x20xp_xxs_sfp_id[] = {
    { "x20xp_sfp_eeprom", sfp_group },
    {}
};
MODULE_DEVICE_TABLE(i2c, x20xp_xxs_sfp_id);

static struct i2c_driver x20xp_xxs_sfp_driver = {
    .driver = {
        .name     = "x20xp_sfp_eeprom",
        .owner = THIS_MODULE,
    },
    .probe        = x20xp_xxs_sfp_device_probe,
    .remove       = x20xp_xxs_sfp_device_remove,
    .id_table     = x20xp_xxs_sfp_id,
};

static int __init x20xp_xxs_sfp_init(void)
{
    int i;

    for(i=0; i<MAX_PORT_NUM; i++)
    {
        sprintf(SFP_EEPROM_MAPPING[i], "sfp%d_eeprom", i+1);
    }

    if (!io_limit) {
        pr_err("optoe: io_limit must not be 0!\n");
        return -EINVAL;
    }

    io_limit = rounddown_pow_of_two(io_limit);

    return i2c_add_driver(&x20xp_xxs_sfp_driver);
}

static void __exit x20xp_xxs_sfp_exit(void)
{
    i2c_del_driver(&x20xp_xxs_sfp_driver);
}

MODULE_AUTHOR(" AF inc. ");
MODULE_DESCRIPTION("x20xp_xxs_sfp_eeprom driver");
MODULE_LICENSE("GPL");

module_init(x20xp_xxs_sfp_init);
module_exit(x20xp_xxs_sfp_exit);
