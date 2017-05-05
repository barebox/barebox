/*
 * at25.c -- support most SPI EEPROMs, such as Atmel AT25 models
 *
 * Copyright (C) 2011 Hubert Feurstein <h.feurstein@gmail.com>
 *
 * based on linux driver by:
 * Copyright (C) 2006 David Brownell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <common.h>
#include <init.h>
#include <clock.h>
#include <driver.h>
#include <errno.h>
#include <xfuncs.h>
#include <malloc.h>
#include <spi/spi.h>
#include <spi/eeprom.h>

/*
 * NOTE: this is an *EEPROM* driver.  The vagaries of product naming
 * mean that some AT25 products are EEPROMs, and others are FLASH.
 * Handle FLASH chips with the drivers/mtd/devices/m25p80.c driver,
 * not this one!
 */

struct at25_data {
	struct cdev		cdev;
	struct spi_device	*spi;
	struct spi_eeprom	chip;
	unsigned		addrlen;
};

#define to_at25_data(cdev)		((struct at25_data *)(cdev)->priv)

#define	AT25_WREN	0x06		/* latch the write enable */
#define	AT25_WRDI	0x04		/* reset the write enable */
#define	AT25_RDSR	0x05		/* read status register */
#define	AT25_WRSR	0x01		/* write status register */
#define	AT25_READ	0x03		/* read byte(s) */
#define	AT25_WRITE	0x02		/* write byte(s)/sector */

#define	AT25_SR_nRDY	0x01		/* nRDY = write-in-progress */
#define	AT25_SR_WEN	0x02		/* write enable (latched) */
#define	AT25_SR_BP0	0x04		/* BP for software writeprotect */
#define	AT25_SR_BP1	0x08
#define	AT25_SR_WPEN	0x80		/* writeprotect enable */


#define EE_MAXADDRLEN	3		/* 24 bit addresses, up to 2 MBytes */

/* Specs often allow 5 msec for a page write, sometimes 20 msec;
 * it's important to recover from write timeouts.
 */
#define	EE_TIMEOUT	(25 * MSECOND)
#define DRIVERNAME	"at25x"

/*-------------------------------------------------------------------------*/

#define	io_limit	PAGE_SIZE	/* bytes */

static ssize_t at25_ee_read(struct cdev *cdev,
			    void *buf,
			    size_t count,
			    loff_t offset,
			    ulong flags)
{
	u8			command[EE_MAXADDRLEN + 1];
	u8			*cp;
	ssize_t			status;
	struct spi_transfer	t[2];
	struct spi_message	m;
	struct at25_data	*at25 = to_at25_data(cdev);

	if (unlikely(offset >= at25->chip.size))
		return 0;
	if ((offset + count) > at25->chip.size)
		count = at25->chip.size - offset;
	if (unlikely(!count))
		return count;

	cp = command;
	*cp++ = AT25_READ;

	/* 8/16/24-bit address is written MSB first */
	switch (at25->addrlen) {
	default:	/* case 3 */
		*cp++ = offset >> 16;
	case 2:
		*cp++ = offset >> 8;
	case 1:
	case 0:	/* can't happen: for better codegen */
		*cp++ = offset >> 0;
	}

	spi_message_init(&m);
	memset(t, 0, sizeof t);

	t[0].tx_buf = command;
	t[0].len = at25->addrlen + 1;
	spi_message_add_tail(&t[0], &m);

	t[1].rx_buf = buf;
	t[1].len = count;
	spi_message_add_tail(&t[1], &m);

	/* Read it all at once.
	 *
	 * REVISIT that's potentially a problem with large chips, if
	 * other devices on the bus need to be accessed regularly or
	 * this chip is clocked very slowly
	 */
	status = spi_sync(at25->spi, &m);
	dev_dbg(at25->cdev.dev,
		"read %zd bytes at %llu --> %zd\n",
		count, offset, status);

	return status ? status : count;
}

static ssize_t at25_ee_write(struct cdev *cdev,
			     const void *buf,
			     size_t count,
			     loff_t off,
			     ulong flags)
{
	ssize_t			status = 0;
	unsigned		written = 0;
	unsigned		buf_size;
	u8			*bounce;
	struct at25_data	*at25 = to_at25_data(cdev);

	if (unlikely(off >= at25->chip.size))
		return -EFBIG;
	if ((off + count) > at25->chip.size)
		count = at25->chip.size - off;
	if (unlikely(!count))
		return count;

	/* Temp buffer starts with command and address */
	buf_size = at25->chip.page_size;
	if (buf_size > io_limit)
		buf_size = io_limit;
	bounce = xmalloc(buf_size + at25->addrlen + 1);

	/* For write, rollover is within the page ... so we write at
	 * most one page, then manually roll over to the next page.
	 */
	bounce[0] = AT25_WRITE;

	do {
		uint64_t	start_time;
		unsigned	segment;
		unsigned	offset = (unsigned) off;
		u8		*cp = bounce + 1;
		int		sr;

		*cp = AT25_WREN;
		status = spi_write(at25->spi, cp, 1);
		if (status < 0) {
			dev_dbg(at25->cdev.dev, "WREN --> %d\n",
					(int) status);
			break;
		}

		/* 8/16/24-bit address is written MSB first */
		switch (at25->addrlen) {
		default:	/* case 3 */
			*cp++ = offset >> 16;
		case 2:
			*cp++ = offset >> 8;
		case 1:
		case 0:	/* can't happen: for better codegen */
			*cp++ = offset >> 0;
		}

		/* Write as much of a page as we can */
		segment = buf_size - (offset % buf_size);
		if (segment > count)
			segment = count;
		memcpy(cp, buf, segment);
		status = spi_write(at25->spi, bounce,
				segment + at25->addrlen + 1);
		dev_dbg(at25->cdev.dev,
				"write %u bytes at %u --> %d\n",
				segment, offset, (int) status);
		if (status < 0)
			break;

		/* REVISIT this should detect (or prevent) failed writes
		 * to readonly sections of the EEPROM...
		 */

		/* Wait for non-busy status */
		start_time = get_time_ns();

		do {
			sr = spi_w8r8(at25->spi, AT25_RDSR);
			if (sr < 0 || (sr & AT25_SR_nRDY)) {
				dev_dbg(at25->cdev.dev,
					"rdsr --> %d (%02x)\n", sr, sr);
				continue;
			}
			if (!(sr & AT25_SR_nRDY))
				break;
		} while (!is_timeout(start_time, EE_TIMEOUT));

		if ((sr < 0) || (sr & AT25_SR_nRDY)) {
			dev_err(at25->cdev.dev,
				"write %d bytes offset %d, "
				"timeout after %u msecs\n",
				segment, offset,
				(unsigned int)(get_time_ns() - start_time) /
					1000000);
			status = -ETIMEDOUT;
			break;
		}

		off += segment;
		buf += segment;
		count -= segment;
		written += segment;

	} while (count > 0);

	free(bounce);
	return written ? written : status;
}

static struct file_operations at25_fops = {
	.read	= at25_ee_read,
	.write	= at25_ee_write,
	.lseek	= dev_lseek_default,
};

static int at25_np_to_chip(struct device_d *dev,
			   struct device_node *np,
			   struct spi_eeprom *chip)
{
	u32 val;

	if (!IS_ENABLED(CONFIG_OFDEVICE))
		return -ENODEV;

	memset(chip, 0, sizeof(*chip));
	strncpy(chip->name, np->name, sizeof(chip->name));

	if (of_property_read_u32(np, "size", &val) == 0 ||
	    of_property_read_u32(np, "at25,byte-len", &val) == 0) {
		chip->size = val;
	} else {
		dev_err(dev, "Error: missing \"size\" property\n");
		return -ENODEV;
	}

	if (of_property_read_u32(np, "pagesize", &val) == 0 ||
	    of_property_read_u32(np, "at25,page-size", &val) == 0) {
		chip->page_size = (u16)val;
	} else {
		dev_err(dev, "Error: missing \"pagesize\" property\n");
		return -ENODEV;
	}

	if (of_property_read_u32(np, "at25,addr-mode", &val) == 0) {
		chip->flags = (u16)val;
	} else {
		if (of_property_read_u32(np, "address-width", &val)) {
			dev_err(dev,
				"Error: missing \"address-width\" property\n");
			return -ENODEV;
		}
		switch (val) {
		case 8:
			chip->flags |= EE_ADDR1;
			break;
		case 16:
			chip->flags |= EE_ADDR2;
			break;
		case 24:
			chip->flags |= EE_ADDR3;
			break;
		default:
			dev_err(dev,
				"Error: bad \"address-width\" property: %u\n",
				val);
			return -ENODEV;
		}
		if (of_property_read_bool(np, "read-only"))
			chip->flags |= EE_READONLY;
	}
	return 0;
}

static int at25_probe(struct device_d *dev)
{
	int err, sr;
	int addrlen;
	struct at25_data *at25 = NULL;
	struct spi_eeprom *chip;

	at25 = xzalloc(sizeof(*at25));

	/* Chip description */
	if (dev->device_node) {
		err = at25_np_to_chip(dev, dev->device_node, &at25->chip);
		if (err)
			goto fail;
	} else {
		chip = dev->platform_data;
		if (!chip) {
			dev_dbg(dev, "no chip description\n");
			err =  -ENODEV;
			goto fail;
		}
		at25->chip = *chip;
	}

	/* For now we only support 8/16/24 bit addressing */
	if (at25->chip.flags & EE_ADDR1) {
		addrlen = 1;
	} else if (at25->chip.flags & EE_ADDR2) {
		addrlen = 2;
	} else if (at25->chip.flags & EE_ADDR3) {
		addrlen = 3;
	} else {
		dev_dbg(dev, "unsupported address type\n");
		err = -EINVAL;
		goto fail;
	}

	at25->addrlen = addrlen;
	at25->spi = dev->type_data;
	at25->spi->mode = SPI_MODE_0;
	at25->spi->bits_per_word = 8;
	at25->cdev.ops = &at25_fops;
	at25->cdev.size = at25->chip.size;
	at25->cdev.dev = dev;
	at25->cdev.name = at25->chip.name[0] ? at25->chip.name : DRIVERNAME;
	at25->cdev.priv = at25;

	/* Ping the chip ... the status register is pretty portable,
	 * unlike probing manufacturer IDs.  We do expect that system
	 * firmware didn't write it in the past few milliseconds!
	 */
	sr = spi_w8r8(at25->spi, AT25_RDSR);
	if (sr < 0 || sr & AT25_SR_nRDY) {
		dev_dbg(dev, "rdsr --> %d (%02x)\n", sr, sr);
		err = -ENXIO;
		goto fail;
	}

	err = devfs_create(&at25->cdev);
	if (err)
		goto fail;

	dev_dbg(dev, "%s probed\n", at25->cdev.name);
	of_parse_partitions(&at25->cdev, dev->device_node);
	of_partitions_register_fixup(&at25->cdev);

	return 0;

fail:
	free(at25);

	return err;
}

static __maybe_unused struct of_device_id at25_dt_ids[] = {
	{
		.compatible = "atmel,at25",
	}, {
		/* sentinel */
	}
};

static struct driver_d at25_driver = {
	.name  = DRIVERNAME,
	.of_compatible = DRV_OF_COMPAT(at25_dt_ids),
	.probe = at25_probe,
};
device_spi_driver(at25_driver);
