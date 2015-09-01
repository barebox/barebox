/*
 * at24.c - handle most I2C EEPROMs
 *
 * Copyright (C) 2005-2007 David Brownell
 * Copyright (C) 2008 Wolfram Sang, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <common.h>
#include <init.h>
#include <malloc.h>
#include <clock.h>
#include <driver.h>
#include <xfuncs.h>
#include <errno.h>
#include <linux/math64.h>
#include <linux/log2.h>
#include <i2c/i2c.h>
#include <i2c/at24.h>
#include <gpio.h>
#include <of_gpio.h>

/*
 * I2C EEPROMs from most vendors are inexpensive and mostly interchangeable.
 * Differences between different vendor product lines (like Atmel AT24C or
 * MicroChip 24LC, etc) won't much matter for typical read/write access.
 * There are also I2C RAM chips, likewise interchangeable. One example
 * would be the PCF8570, which acts like a 24c02 EEPROM (256 bytes).
 *
 * However, misconfiguration can lose data. "Set 16-bit memory address"
 * to a part with 8-bit addressing will overwrite data. Writing with too
 * big a page size also loses data. And it's not safe to assume that the
 * conventional addresses 0x50..0x57 only hold eeproms; a PCF8563 RTC
 * uses 0x51, for just one example.
 *
 * So this driver uses "new style" I2C driver binding, expecting to be
 * told what devices exist. That may be in arch/X/mach-Y/board-Z.c or
 * similar kernel-resident tables; or, configuration data coming from
 * a bootloader.
 *
 * Other than binding model, current differences from "eeprom" driver are
 * that this one handles write access and isn't restricted to 24c02 devices.
 * It also handles larger devices (32 kbit and up) with two-byte addresses,
 * which won't work on pure SMBus systems.
 */

struct at24_data {
	struct at24_platform_data chip;

	struct cdev		cdev;
	struct file_operations	fops;

	u8 *writebuf;
	unsigned write_max;
	unsigned num_addresses;
	int wp_gpio;
	int wp_active_low;

	/*
	 * Some chips tie up multiple I2C addresses; dummy devices reserve
	 * them for us.
	 */
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
static unsigned io_limit = 128;

/*
 * Specs often allow 5 msec for a page write, sometimes 20 msec;
 * it's important to recover from write timeouts.
 */
static unsigned write_timeout = 25;

#define AT24_SIZE_BYTELEN 5
#define AT24_SIZE_FLAGS 8

#define AT24_BITMASK(x) (BIT(x) - 1)

/* create non-zero magic value for given eeprom parameters */
#define AT24_DEVICE_MAGIC(_len, _flags) 		\
	((1 << AT24_SIZE_FLAGS | (_flags)) 		\
	    << AT24_SIZE_BYTELEN | ilog2(_len))

static struct platform_device_id at24_ids[] = {
	/* needs 8 addresses as A0-A2 are ignored */
	{ "24c00", AT24_DEVICE_MAGIC(128 / 8, AT24_FLAG_TAKE8ADDR) },
	/* old variants can't be handled with this generic entry! */
	{ "24c01", AT24_DEVICE_MAGIC(1024 / 8, 0) },
	{ "24c02", AT24_DEVICE_MAGIC(2048 / 8, 0) },
	/* spd is a 24c02 in memory DIMMs */
	{ "spd", AT24_DEVICE_MAGIC(2048 / 8,
		AT24_FLAG_READONLY | AT24_FLAG_IRUGO) },
	{ "24c04", AT24_DEVICE_MAGIC(4096 / 8, 0) },
	/* 24rf08 quirk is handled at i2c-core */
	{ "24c08", AT24_DEVICE_MAGIC(8192 / 8, 0) },
	{ "24c16", AT24_DEVICE_MAGIC(16384 / 8, 0) },
	{ "24c32", AT24_DEVICE_MAGIC(32768 / 8, AT24_FLAG_ADDR16) },
	{ "24c64", AT24_DEVICE_MAGIC(65536 / 8, AT24_FLAG_ADDR16) },
	{ "24c128", AT24_DEVICE_MAGIC(131072 / 8, AT24_FLAG_ADDR16) },
	{ "24c256", AT24_DEVICE_MAGIC(262144 / 8, AT24_FLAG_ADDR16) },
	{ "24c512", AT24_DEVICE_MAGIC(524288 / 8, AT24_FLAG_ADDR16) },
	{ "24c1024", AT24_DEVICE_MAGIC(1048576 / 8, AT24_FLAG_ADDR16) },
	{ "at24", 0 },
	{ /* END OF LIST */ }
};

/*-------------------------------------------------------------------------*/

/*
 * This routine supports chips which consume multiple I2C addresses. It
 * computes the addressing information to be used for a given r/w request.
 * Assumes that sanity checks for offset happened at sysfs-layer.
 */
static struct i2c_client *at24_translate_offset(struct at24_data *at24,
		unsigned *offset)
{
	unsigned i;

	if (at24->chip.flags & AT24_FLAG_ADDR16) {
		i = *offset >> 16;
		*offset &= 0xffff;
	} else {
		i = *offset >> 8;
		*offset &= 0xff;
	}

	return at24->client[i];
}

static ssize_t at24_eeprom_read(struct at24_data *at24, char *buf,
		unsigned offset, size_t count)
{
	struct i2c_msg msg[2];
	u8 msgbuf[2];
	struct i2c_client *client;
	int status, i;
	uint64_t start, read_time;

	memset(msg, 0, sizeof(msg));

	/*
	 * REVISIT some multi-address chips don't rollover page reads to
	 * the next slave address, so we may need to truncate the count.
	 * Those chips might need another quirk flag.
	 *
	 * If the real hardware used four adjacent 24c02 chips and that
	 * were misconfigured as one 24c08, that would be a similar effect:
	 * one "eeprom" file not four, but larger reads would fail when
	 * they crossed certain pages.
	 */

	/*
	 * Slave address and byte offset derive from the offset. Always
	 * set the byte address; on a multi-master board, another master
	 * may have changed the chip's "current" address pointer.
	 */
	client = at24_translate_offset(at24, &offset);

	if (count > io_limit)
		count = io_limit;

	i = 0;
	if (at24->chip.flags & AT24_FLAG_ADDR16)
		msgbuf[i++] = offset >> 8;
	msgbuf[i++] = offset;

	msg[0].addr = client->addr;
	msg[0].buf = msgbuf;
	msg[0].len = i;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = buf;
	msg[1].len = count;

	/*
	 * Reads fail if the previous write didn't complete yet. We may
	 * loop a few times until this one succeeds, waiting at least
	 * long enough for one entire page write to work.
	 */
	start = get_time_ns();
	do {
		read_time = get_time_ns();
		status = i2c_transfer(client->adapter, msg, 2);
		if (status == 2)
			status = count;
		dev_dbg(&client->dev, "read %zu@%d --> %d (%llu)\n",
				count, offset, status, read_time);

		if (status == count)
			return count;

		/* REVISIT: at HZ=100, this is sloooow */
		mdelay(1);
	} while (!is_timeout(start, write_timeout * MSECOND));

	return -ETIMEDOUT;
}

static ssize_t at24_read(struct at24_data *at24,
		char *buf, loff_t off, size_t count)
{
	ssize_t retval = 0;

	if (unlikely(!count))
		return count;

	/*
	 * Read data from chip, protecting against concurrent updates
	 * from this host, but not from other I2C masters.
	 */
	while (count) {
		ssize_t	status;

		status = at24_eeprom_read(at24, buf, off, count);
		if (status <= 0) {
			if (retval == 0)
				retval = status;
			break;
		}
		buf += status;
		off += status;
		count -= status;
		retval += status;
	}

	return retval;
}

static ssize_t at24_cdev_read(struct cdev *cdev, void *buf, size_t count,
		loff_t off, ulong flags)
{
	struct at24_data *at24 = cdev->priv;

	return at24_read(at24, buf, off, count);
}

/*
 * Note that if the hardware write-protect pin is pulled high, the whole
 * chip is normally write protected. But there are plenty of product
 * variants here, including OTP fuses and partial chip protect.
 *
 * We only use page mode writes; the alternative is sloooow. This routine
 * writes at most one page.
 */
static ssize_t at24_eeprom_write(struct at24_data *at24, const char *buf,
		unsigned offset, size_t count)
{
	struct i2c_client *client;
	struct i2c_msg msg;
	ssize_t status;
	uint64_t start, write_time;
	unsigned next_page;
	int i = 0;

	/* Get corresponding I2C address and adjust offset */
	client = at24_translate_offset(at24, &offset);

	/* write_max is at most a page */
	if (count > at24->write_max)
		count = at24->write_max;

	/* Never roll over backwards, to the start of this page */
	next_page = roundup(offset + 1, at24->chip.page_size);
	if (offset + count > next_page)
		count = next_page - offset;


	msg.addr = client->addr;
	msg.flags = 0;

	/* msg.buf is u8 and casts will mask the values */
	msg.buf = at24->writebuf;
	if (at24->chip.flags & AT24_FLAG_ADDR16)
		msg.buf[i++] = offset >> 8;

	msg.buf[i++] = offset;
	memcpy(&msg.buf[i], buf, count);
	msg.len = i + count;

	/*
	 * Writes fail if the previous one didn't complete yet. We may
	 * loop a few times until this one succeeds, waiting at least
	 * long enough for one entire page write to work.
	 */
	start = get_time_ns();
	do {
		write_time = get_time_ns();
		status = i2c_transfer(client->adapter, &msg, 1);
		if (status == 1)
			status = count;
		dev_dbg(&client->dev, "write %zu@%d --> %zd (%llu)\n",
				count, offset, status, write_time);

		if (status == count)
			return count;

		/* REVISIT: at HZ=100, this is sloooow */
		mdelay(1);
	} while (!is_timeout(start, write_timeout * MSECOND));

	return -ETIMEDOUT;
}

static ssize_t at24_write(struct at24_data *at24, const char *buf, loff_t off,
			  size_t count)
{
	ssize_t retval = 0;

	if (unlikely(!count))
		return count;

	while (count) {
		ssize_t	status;

		status = at24_eeprom_write(at24, buf, off, count);
		if (status <= 0) {
			if (retval == 0)
				retval = status;
			break;
		}
		buf += status;
		off += status;
		count -= status;
		retval += status;
	}

	return retval;
}

static ssize_t at24_cdev_write(struct cdev *cdev, const void *buf, size_t count,
		loff_t off, ulong flags)
{
	struct at24_data *at24 = cdev->priv;

	return at24_write(at24, buf, off, count);
}

static ssize_t at24_cdev_protect(struct cdev *cdev, size_t count, loff_t offset,
		int prot)
{
	struct at24_data *at24 = cdev->priv;

	if (!gpio_is_valid(at24->wp_gpio))
		return -EOPNOTSUPP;

	prot = !!prot;
	if (at24->wp_active_low)
		prot = !prot;

	gpio_set_value(at24->wp_gpio, prot);

	udelay(50);

	return 0;
}

static int at24_probe(struct device_d *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct at24_platform_data chip;
	bool writable;
	struct at24_data *at24;
	int err;
	unsigned i, num_addresses;

	if (dev->platform_data) {
		chip = *(struct at24_platform_data *)dev->platform_data;
	} else {
		unsigned long magic;

		err = dev_get_drvdata(dev, (const void **)&magic);
		if (err)
			return err;

		chip.byte_len = BIT(magic & AT24_BITMASK(AT24_SIZE_BYTELEN));
		magic >>= AT24_SIZE_BYTELEN;
		chip.flags = magic & AT24_BITMASK(AT24_SIZE_FLAGS);
		/*
		 * This is slow, but we can't know all eeproms, so we better
		 * play safe. Specifying custom eeprom-types via platform_data
		 * is recommended anyhow.
		 */
		chip.page_size = 1;
	}

	if (!is_power_of_2(chip.byte_len))
		dev_warn(&client->dev,
			"byte_len looks suspicious (no power of 2)!\n");
	if (!chip.page_size) {
		dev_err(&client->dev, "page_size must not be 0!\n");
		err = -EINVAL;
		goto err_out;
	}
	if (!is_power_of_2(chip.page_size))
		dev_warn(&client->dev,
			"page_size looks suspicious (no power of 2)!\n");

	if (chip.flags & AT24_FLAG_TAKE8ADDR)
		num_addresses = 8;
	else
		num_addresses =	DIV_ROUND_UP(chip.byte_len,
			(chip.flags & AT24_FLAG_ADDR16) ? 65536 : 256);

	at24 = xzalloc(sizeof(struct at24_data) +
		num_addresses * sizeof(struct i2c_client *));

	at24->chip = chip;
	at24->num_addresses = num_addresses;
	at24->cdev.name = asprintf("eeprom%d", dev->id);
	at24->cdev.priv = at24;
	at24->cdev.dev = dev;
	at24->cdev.ops = &at24->fops;
	at24->fops.lseek = dev_lseek_default;
	at24->fops.read	= at24_cdev_read,
	at24->fops.protect = at24_cdev_protect,
	at24->cdev.size = chip.byte_len;

	writable = !(chip.flags & AT24_FLAG_READONLY);
	if (writable) {
		unsigned write_max = chip.page_size;

		at24->fops.write = at24_cdev_write;

		if (write_max > io_limit)
			write_max = io_limit;
		at24->write_max = write_max;

		/* buffer (data + address at the beginning) */
		at24->writebuf = xmalloc(write_max + 2);
	}

	at24->wp_gpio = -1;
	if (dev->device_node) {
		enum of_gpio_flags flags;
		at24->wp_gpio = of_get_named_gpio_flags(dev->device_node,
				"wp-gpios", 0, &flags);
		if (gpio_is_valid(at24->wp_gpio)) {
			at24->wp_active_low = flags & OF_GPIO_ACTIVE_LOW;
			gpio_request(at24->wp_gpio, "eeprom-wp");
			gpio_direction_output(at24->wp_gpio,
					!at24->wp_active_low);
		}
	}

	at24->client[0] = client;

	/* use dummy devices for multiple-address chips */
	for (i = 1; i < num_addresses; i++) {
		at24->client[i] = i2c_new_dummy(client->adapter,
					client->addr + i);
		if (!at24->client[i]) {
			dev_err(&client->dev, "address 0x%02x unavailable\n",
					client->addr + i);
			err = -EADDRINUSE;
			goto err_clients;
		}
	}

	devfs_create(&at24->cdev);

	of_parse_partitions(&at24->cdev, dev->device_node);

	return 0;

err_clients:
	gpio_free(at24->wp_gpio);
	kfree(at24->writebuf);
	kfree(at24);
err_out:
	dev_dbg(&client->dev, "probe error %d\n", err);
	return err;

}

static struct driver_d at24_driver = {
	.name		= "at24",
	.probe		= at24_probe,
	.id_table	= at24_ids,
};

static int at24_init(void)
{
	i2c_driver_register(&at24_driver);
	return 0;
}
device_initcall(at24_init);
