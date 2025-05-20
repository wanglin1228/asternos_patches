/*
 * cx206y_48t_sfp.c - A driver to read and write the EEPROM on optical transceivers
 * (SFP and QSFP)
 *
 * Copyright (C) 2014 Cumulus networks Inc.
 * Copyright (C) 2017 Finisar Corp.
 * Copyright (C) 2019 asterfusion Corp. Wang_Lin <wanglin@asterfusion.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Freeoftware Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*
 *	Description:
 *	a) Optical transceiver EEPROM read/write transactions are just like
 *		the at24 eeproms managed by the at24.c i2c driver
 *	b) The register/memory layout is up to 256 128 byte pages defined by
 *		a "pages valid" register and switched via a "page select"
 *		register as explained in below diagram.
 *	c) 256 bytes are mapped at a time. 'Lower page 00h' is the first 128
 *	        bytes of address space, and always references the same
 *	        location, independent of the page select register.
 *	        All mapped pages are mapped into the upper 128 bytes
 *	        (offset 128-255) of the i2c address.
 *	d) Devices with one I2C address (eg QSFP) use I2C address 0x50
 *		(A0h in the spec), and map all pages in the upper 128 bytes
 *		of that address.
 *	e) Devices with two I2C addresses (eg SFP) have 256 bytes of data
 *		at I2C address 0x50, and 256 bytes of data at I2C address
 *		0x51 (A2h in the spec).  Page selection and paged access
 *		only apply to this second I2C address (0x51).
 *	e) The address space is presented, by the driver, as a linear
 *	        address space.  For devices with one I2C client at address
 *	        0x50 (eg QSFP), offset 0-127 are in the lower
 *	        half of address 50/A0h/client[0].  Offset 128-255 are in
 *	        page 0, 256-383 are page 1, etc.  More generally, offset
 *	        'n' resides in page (n/128)-1.  ('page -1' is the lower
 *	        half, offset 0-127).
 *	f) For devices with two I2C clients at address 0x50 and 0x51 (eg SFP),
 *		the address space places offset 0-127 in the lower
 *	        half of 50/A0/client[0], offset 128-255 in the upper
 *	        half.  Offset 256-383 is in the lower half of 51/A2/client[1].
 *	        Offset 384-511 is in page 0, in the upper half of 51/A2/...
 *	        Offset 512-639 is in page 1, in the upper half of 51/A2/...
 *	        Offset 'n' is in page (n/128)-3 (for n > 383)
 *
 *	                    One I2c addressed (eg QSFP) Memory Map
 *
 *	                    2-Wire Serial Address: 1010000x
 *
 *	                    Lower Page 00h (128 bytes)
 *	                    =====================
 *	                   |                     |
 *	                   |                     |
 *	                   |                     |
 *	                   |                     |
 *	                   |                     |
 *	                   |                     |
 *	                   |                     |
 *	                   |                     |
 *	                   |                     |
 *	                   |                     |
 *	                   |Page Select Byte(127)|
 *	                    =====================
 *	                              |
 *	                              |
 *	                              |
 *	                              |
 *	                              V
 *	     ------------------------------------------------------------
 *	    |                 |                  |                       |
 *	    |                 |                  |                       |
 *	    |                 |                  |                       |
 *	    |                 |                  |                       |
 *	    |                 |                  |                       |
 *	    |                 |                  |                       |
 *	    |                 |                  |                       |
 *	    |                 |                  |                       |
 *	    |                 |                  |                       |
 *	    V                 V                  V                       V
 *	 ------------   --------------      ---------------     --------------
 *	|            | |              |    |               |   |              |
 *	|   Upper    | |     Upper    |    |     Upper     |   |    Upper     |
 *	|  Page 00h  | |    Page 01h  |    |    Page 02h   |   |   Page 03h   |
 *	|            | |   (Optional) |    |   (Optional)  |   |  (Optional   |
 *	|            | |              |    |               |   |   for Cable  |
 *	|            | |              |    |               |   |  Assemblies) |
 *	|    ID      | |     AST      |    |      User     |   |              |
 *	|  Fields    | |    Table     |    |   EEPROM Data |   |              |
 *	|            | |              |    |               |   |              |
 *	|            | |              |    |               |   |              |
 *	|            | |              |    |               |   |              |
 *	 ------------   --------------      ---------------     --------------
 *
 * The SFF 8436 (QSFP) spec only defines the 4 pages described above.
 * In anticipation of future applications and devices, this driver
 * supports access to the full architected range, 256 pages.
 *
 * The CMIS (Common Management Interface Specification) defines use of
 * considerably more pages (at least to page 0xAF), which this driver
 * supports.
 *
 * NOTE: This version of the driver ONLY SUPPORTS BANK 0 PAGES on CMIS
 * devices.
 *
 **/

#undef aster_DEBUG
/*#define aster_DEBUG*/
#ifdef aster_DEBUG
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

#define NUM_ADDRESS 2

/* The maximum length of a port name */
#define MAX_PORT_NAME_LEN 20

/* fundamental unit of addressing for EEPROM */
#define SFP_PAGE_SIZE 128

/*
 * Single address devices (eg QSFP) have 256 pages, plus the unpaged
 * low 128 bytes.  If the device does not support paging, it is
 * only 2 'pages' long.
 */
#define SFP_ARCH_PAGES 256
#define ONE_ADDR_EEPROM_SIZE ((1 + SFP_ARCH_PAGES) * SFP_PAGE_SIZE)
#define ONE_ADDR_EEPROM_UNPAGED_SIZE (2 * SFP_PAGE_SIZE)

/*
 * Dual address devices (eg SFP) have 256 pages, plus the unpaged
 * low 128 bytes, plus 256 bytes at 0x50.  If the device does not
 * support paging, it is 4 'pages' long.
 */
#define TWO_ADDR_EEPROM_SIZE ((3 + SFP_ARCH_PAGES) * SFP_PAGE_SIZE)
#define TWO_ADDR_EEPROM_UNPAGED_SIZE (4 * SFP_PAGE_SIZE)
#define TWO_ADDR_NO_0X51_SIZE (2 * SFP_PAGE_SIZE)

/*
 * flags to distinguish one-address (QSFP family) from two-address (SFP family)
 * If the family is not known, figure it out when the device is accessed
 */
#define ONE_ADDR 1
#define TWO_ADDR 2
#define CMIS_ADDR 3

/* a few constants to find our way around the EEPROM */\
#define SFP_EEPROM_A0_ADDR 	0x50
#define SFP_EEPROM_A2_ADDR 	0x51
#define SFP_PAGE_SELECT_REG   0x7F
#define ONE_ADDR_PAGEABLE_REG 0x02
#define QSFP_NOT_PAGEABLE (1<<2)
#define CMIS_NOT_PAGEABLE (1<<7)
#define TWO_ADDR_PAGEABLE_REG 0x40
#define TWO_ADDR_PAGEABLE (1<<4)
#define TWO_ADDR_0X51_REG 92
#define TWO_ADDR_0X51_SUPP (1<<6)
#define SFP_ID_REG 0
#define SFP_READ_OP 0
#define SFP_WRITE_OP 1
#define SFP_EOF 0  /* used for access beyond end of device */				
#define CPLD_SFP_NUM                6
#define MAX_PORT_NUM				CPLD_SFP_NUM
#define CPLD_ADDRESS                0x75
#define SFP_SCL_BASE           		0x7

#define GET_BIT(data, bit, value)   value = (data >> bit) & 0x1
#define SET_BIT(data, bit)          data |= (1 << bit)
#define CLEAR_BIT(data, bit)        data &= ~(1 << bit)

enum cpld_croups { cpld_group};

struct cx206y_48t_sfp_platform_data {
	u32		byte_len;		/* size (sum of all addr) */
	u16		page_size;		/* for writes */
	u8		flags;
	void		*dummy1;		/* backward compatibility */
	void		*dummy2;		/* backward compatibility */

	/* dev_class: ONE_ADDR (QSFP) or TWO_ADDR (SFP) */
	int dev_class;
	unsigned int write_max;
};

struct cx206y_48t_sfp_data {
	struct cx206y_48t_sfp_platform_data chip[MAX_PORT_NUM];

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

/*
 * specs often allow 5 msec for a page write, sometimes 20 msec;
 * it's important to recover from write timeouts.
 */
static unsigned int write_timeout = 25;

static char SFP_CPLD_GROUP_MAPPING[CPLD_SFP_NUM][MAX_PORT_NAME_LEN]={0};

extern int cx206y_48t_cpld_read(unsigned short cpld_addr, u8 reg);
extern int cx206y_48t_cpld_write(unsigned short cpld_addr, u8 reg, u8 value);

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

static uint8_t cx206y_48t_sfp_translate_offset(struct cx206y_48t_sfp_data *sfp,
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

	return page;  /* note also returning clicx206y_48t_sfp_translate_offsetent and offset */
}

static ssize_t cx206y_48t_sfp_eeprom_read(struct cx206y_48t_sfp_data *sfp,
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

static ssize_t cx206y_48t_sfp_eeprom_write(struct cx206y_48t_sfp_data *sfp,
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

static ssize_t cx206y_48t_sfp_eeprom_update_client(struct cx206y_48t_sfp_data *sfp,
				char *buf, loff_t off, size_t count, int num)
{
	struct i2c_client *client;
	ssize_t retval = 0;
	uint8_t page = 0;
	loff_t phy_offset = off;
	int ret = 0;

	page = cx206y_48t_sfp_translate_offset(sfp, &phy_offset, &client, num);

	dev_dbg(&client->dev,
		"%s off %lld  page:%d phy_offset:%lld, count:%ld\n",
		__func__, off, page, phy_offset, (long int) count);
	if (page > 0) {
		ret = cx206y_48t_sfp_eeprom_write(sfp, client, &page,
			SFP_PAGE_SELECT_REG, 1);
		if (ret < 0) {
			dev_dbg(&client->dev,
				"Write page register for page %d failed ret:%d!\n",
					page, ret);
			return ret;
		}
	}

	while (count) {
		ssize_t	status;

		status =  cx206y_48t_sfp_eeprom_read(sfp, client,
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
		ret = cx206y_48t_sfp_eeprom_write(sfp, client, &page,
			SFP_PAGE_SELECT_REG, 1);
		if (ret < 0) {
			dev_err(&client->dev,
				"Restore page register to 0 failed:%d!\n", ret);
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
static ssize_t cx206y_48t_sfp_page_legal(struct cx206y_48t_sfp_data *sfp,
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
		status = cx206y_48t_sfp_eeprom_read(sfp, client, &regval,
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
			status = cx206y_48t_sfp_eeprom_read(sfp, client, &regval,
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
		status = cx206y_48t_sfp_eeprom_read(sfp, client, &regval,
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

static ssize_t cx206y_48t_sfp_read(struct cx206y_48t_sfp_data *sfp,
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

	/*
	 * Confirm this access fits within the device suppored addr range
	 */
	status = cx206y_48t_sfp_page_legal(sfp, off, len, num);
	if ((status == SFP_EOF) || (status < 0)) {
		mutex_unlock(&sfp->lock);
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
		status = cx206y_48t_sfp_eeprom_update_client(sfp, buf,
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

	return retval;
}

static ssize_t 
cx206y_48t_sfp_bin_read(struct file *filp, struct kobject *kobj,
		struct bin_attribute *attr,
		char *buf, loff_t off, size_t count)
{
	int i;
	struct cx206y_48t_sfp_data *sfp;

	sfp = dev_get_drvdata(container_of(kobj, struct device, kobj));

	/*SFP 1-6*/
	for(i=0; i<CPLD_SFP_NUM; i++)
	{
		if(!strcmp(attr->attr.name, SFP_CPLD_GROUP_MAPPING[i]))
		{
			cx206y_48t_cpld_write(CPLD_ADDRESS, SFP_SCL_BASE, i+1);
			goto check_done;
		}
	}

check_done:
	return cx206y_48t_sfp_read(sfp, buf, off, count, i);
}

#define SFP_EEPROM_ATTR(_num)    \
        static struct bin_attribute sfp##_num##_eeprom_attr = {   \
                .attr = {   \
                        .name =  __stringify(sfp##_num##_eeprom),  \
                        .mode = S_IRUGO\
                },  \
                .size = TWO_ADDR_EEPROM_SIZE,    \
                .read = cx206y_48t_sfp_bin_read,   \
                }

SFP_EEPROM_ATTR(49);SFP_EEPROM_ATTR(50);SFP_EEPROM_ATTR(51);SFP_EEPROM_ATTR(52);SFP_EEPROM_ATTR(53);SFP_EEPROM_ATTR(54);

static struct bin_attribute *cx206y_48t_cpld_sfp_epprom_attributes[] = {
	&sfp49_eeprom_attr, &sfp50_eeprom_attr, &sfp51_eeprom_attr, &sfp52_eeprom_attr, &sfp53_eeprom_attr, &sfp54_eeprom_attr,
	NULL
};

static const struct attribute_group cx206y_48t_sfp_group = { .bin_attrs = cx206y_48t_cpld_sfp_epprom_attributes};

static int cx206y_48t_sfp_device_probe(struct i2c_client *client, const struct i2c_device_id *dev_id)
{
	int i, err;
	struct cx206y_48t_sfp_platform_data chip[MAX_PORT_NUM];
	struct cx206y_48t_sfp_data *sfp;
	unsigned int write_max = 1;

	if (client->addr != SFP_EEPROM_A0_ADDR) {
		DBG(printk(KERN_ALERT "%s - probe, bad i2c addr: 0x%x\n", __func__, client->addr));
		err = -EINVAL;
		goto exit;
	}

	sfp = kzalloc(sizeof(struct cx206y_48t_sfp_data) +
			NUM_ADDRESS * sizeof(struct i2c_client *),
			GFP_KERNEL);
	if (!sfp) {
		err = -ENOMEM;
		goto exit;
	}

	mutex_init(&sfp->lock);

	switch(dev_id->driver_data)
	{
		case cpld_group:
			for(i=0; i<CPLD_SFP_NUM; i++)
			{
				chip[i].dev_class = TWO_ADDR;
				chip[i].byte_len = TWO_ADDR_EEPROM_SIZE;
			}

			err = sysfs_create_group(&client->dev.kobj, &cx206y_48t_sfp_group);
			if (err)
				goto err_clients;
			break;
		default:
			printk(KERN_ALERT "i2c_check_CPLD failed\n");
	        err = -EIO;
			break;
	}
	
	if (write_max > io_limit)
		write_max = io_limit;

	if (write_max > I2C_SMBUS_BLOCK_MAX)
		write_max = I2C_SMBUS_BLOCK_MAX;

	for(i=0; i<MAX_PORT_NUM; i++)
	{
		chip[i].write_max = write_max;
		sfp->chip[i] = chip[i];
	}

	sfp->driver_data = dev_id->driver_data;
	sfp->client[0] = client;

	/* SFF-8472 spec requires that the second I2C address be 0x51 */
	if (NUM_ADDRESS == 2) {
		sfp->client[1] = i2c_new_dummy_device(client->adapter, SFP_EEPROM_A2_ADDR); //i2c_new_dummy_device in kernel-5.*
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

static int cx206y_48t_sfp_device_remove(struct i2c_client *client)
{
	struct cx206y_48t_sfp_data *sfp;
	int i;

	sfp = i2c_get_clientdata(client);

	switch(sfp->driver_data)
    {
        case cpld_group:
            sysfs_remove_group(&client->dev.kobj, &cx206y_48t_sfp_group);
            break;
        default:
            dev_dbg(&client->dev, "i2c_remove_CPLD failed (0x%x)\n", client->addr);
            break;
    }

    for (i = 1; i < NUM_ADDRESS; i++)
		i2c_unregister_device(sfp->client[i]);

	kfree(sfp);

	return 0;
}

static const struct i2c_device_id cx206y_48t_sfp_id[] = {
    { "cx206y_48t_sfp", cpld_group },
    {}
};
MODULE_DEVICE_TABLE(i2c, cx206y_48t_sfp_id);

static struct i2c_driver cx206y_48t_sfp_driver = {
    .driver = {
        .name     = "cx206y_48t_sfp",
    },
    .probe        = cx206y_48t_sfp_device_probe,
    .remove       = cx206y_48t_sfp_device_remove,
    .id_table     = cx206y_48t_sfp_id,
};

static int __init cx206y_48t_sfp_init(void)
{
	int i;

	/*SFP 1-6*/
	for(i=0; i<CPLD_SFP_NUM; i++)
	{
		sprintf(SFP_CPLD_GROUP_MAPPING[i], "sfp%d_eeprom", i+49);
	}

	if (!io_limit) {
		pr_err("optoe: io_limit must not be 0!\n");
		return -EINVAL;
	}

	io_limit = rounddown_pow_of_two(io_limit);

	return i2c_add_driver(&cx206y_48t_sfp_driver);
}

static void __exit cx206y_48t_sfp_exit(void)
{
	i2c_del_driver(&cx206y_48t_sfp_driver);
}

MODULE_AUTHOR("Wang Lin <wanglin@asterfusion.com>");
MODULE_DESCRIPTION("cx206y_48t_sfp driver");
MODULE_LICENSE("GPL");

module_init(cx206y_48t_sfp_init);
module_exit(cx206y_48t_sfp_exit);

