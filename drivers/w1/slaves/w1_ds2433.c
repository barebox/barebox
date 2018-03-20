/*
 *	w1_ds2433.c - w1 family 23 (DS2433) driver
 *
 * Copyright (c) 2005 Ben Gardner <bgardner@wabtec.com>
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2. See the file COPYING for more details.
 */

#include <init.h>
#include "../w1.h"

#define W1_PAGE_SIZE		32
#define W1_PAGE_BITS		5
#define W1_PAGE_MASK		0x1F

#define W1_F23_TIME		300

#define W1_F23_READ_EEPROM	0xF0
#define W1_F23_WRITE_SCRATCH	0x0F
#define W1_F23_READ_SCRATCH	0xAA
#define W1_F23_COPY_SCRATCH	0x55

static int ds2433_count = 0;
static int ds28ec20_count = 0;

/**
 * Check the file size bounds and adjusts count as needed.
 * This would not be needed if the file size didn't reset to 0 after a write.
 */
static inline size_t ds2433_fix_count(loff_t off, size_t count, size_t size)
{
	if (off > size)
		return 0;

	if ((off + count) > size)
		return (size - off);

	return count;
}

static ssize_t ds2433_cdev_read(struct cdev *cdev, void *buf, size_t count,
		loff_t off, ulong flags)
{
	struct w1_device *dev = cdev->priv;
	struct w1_bus *bus = dev->bus;
	u8 wrbuf[3];

	if ((count = ds2433_fix_count(off, count, cdev->size)) == 0)
		return 0;

	/* read directly from the EEPROM */
	if (w1_reset_select_slave(dev)) {
		count = -EIO;
		goto out_up;
	}

	wrbuf[0] = W1_F23_READ_EEPROM;
	wrbuf[1] = off & 0xff;
	wrbuf[2] = off >> 8;
	w1_write_block(bus, wrbuf, 3);
	w1_read_block(bus, buf, count);

out_up:
	return count;
}

#ifdef CONFIG_W1_SLAVE_DS2433_WRITE
/**
 * Writes to the scratchpad and reads it back for verification.
 * Then copies the scratchpad to EEPROM.
 * The data must be on one page.
 * The master must be locked.
 *
 * @param sl	The slave structure
 * @param addr	Address for the write
 * @param len   length must be <= (W1_PAGE_SIZE - (addr & W1_PAGE_MASK))
 * @param data	The data to write
 * @return	0=Success -1=failure
 */
static int ds2433_write(struct w1_device *dev, int addr, int len, const u8 *data)
{
	struct w1_bus *bus = dev->bus;
	u8 wrbuf[4];
	u8 rdbuf[W1_PAGE_SIZE + 3];
	u8 es = (addr + len - 1) & 0x1f;

	/* Write the data to the scratchpad */
	if (w1_reset_select_slave(dev))
		return -1;

	wrbuf[0] = W1_F23_WRITE_SCRATCH;
	wrbuf[1] = addr & 0xff;
	wrbuf[2] = addr >> 8;

	w1_write_block(bus, wrbuf, 3);
	w1_write_block(bus, data, len);

	/* Read the scratchpad and verify */
	if (w1_reset_select_slave(dev))
		return -1;

	w1_write_8(bus, W1_F23_READ_SCRATCH);
	w1_read_block(bus, rdbuf, len + 3);

	/* Compare what was read against the data written */
	if ((rdbuf[0] != wrbuf[1]) || (rdbuf[1] != wrbuf[2]) ||
	    (rdbuf[2] != es) || (memcmp(data, &rdbuf[3], len) != 0))
		return -1;

	/* Copy the scratchpad to EEPROM */
	if (w1_reset_select_slave(dev))
		return -1;

	wrbuf[0] = W1_F23_COPY_SCRATCH;
	wrbuf[3] = es;
	w1_write_block(bus, wrbuf, 4);

	/* Sleep for 5 ms to wait for the write to complete */
	mdelay(5);

	/* Reset the bus to wake up the EEPROM (this may not be needed) */
	w1_reset_bus(bus);
	return 0;
}

static ssize_t ds2433_cdev_write(struct cdev *cdev, const void *buf, size_t count,
		loff_t off, ulong flags)
{
	struct w1_device *dev = cdev->priv;
	int addr, len, idx;
	const u8 *buf8 = buf;

	if ((count = ds2433_fix_count(off, count, cdev->size)) == 0)
		return 0;

	/* Can only write data to one page at a time */
	idx = 0;
	while (idx < count) {
		addr = off + idx;
		len = W1_PAGE_SIZE - (addr & W1_PAGE_MASK);
		if (len > (count - idx))
			len = count - idx;

		if (ds2433_write(dev, addr, len, &buf8[idx]) < 0) {
			count = -EIO;
			goto out_up;
		}
		idx += len;
	}

out_up:
	return count;
}
#else
#define ds2433_cdev_write NULL
#endif

static struct cdev_operations ds2433_ops = {
	.read	= ds2433_cdev_read,
	.write	= ds2433_cdev_write,
	.lseek	= dev_lseek_default,
};

static int ds2433_cdev_create(struct w1_device *dev, int size, int id)
{
	struct cdev *cdev;

	cdev = xzalloc(sizeof(*cdev));
	cdev->dev	= &dev->dev;
	cdev->priv	= dev;
	cdev->ops	= &ds2433_ops;
	cdev->size	= size;
	cdev->name	= basprintf("%s%d", dev->dev.driver->name, id);
	if (cdev->name == NULL)
		return -ENOMEM;

	return devfs_create(cdev);
}

static int ds2433_probe(struct w1_device *dev)
{
	return ds2433_cdev_create(dev, 512, ds2433_count++);
}

static int ds28ec20_probe(struct w1_device *dev)
{
	return ds2433_cdev_create(dev, 2560, ds28ec20_count++);
}

struct w1_driver ds2433_driver = {
	.drv = {
		.name = "ds2433",
	},
	.probe		= ds2433_probe,
	.fid		= 0x23,
};

struct w1_driver ds28ec20_driver = {
	.drv = {
		.name = "ds28ec20",
	},
	.probe		= ds28ec20_probe,
	.fid		= 0x43,
};

static int w1_ds2433_init(void)
{
	int ret;

	ret = w1_driver_register(&ds2433_driver);
	if (ret)
		return ret;

	return w1_driver_register(&ds28ec20_driver);
}
device_initcall(w1_ds2433_init);
