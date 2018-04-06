/*
 * w1_ds2431.c - w1 family 2d (DS2431) driver
 *
 * Copyright (c) 2008 Bernhard Weirich <bernhard.weirich@riedel.net>
 * Copyright (c) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Heavily inspired by w1_DS2433 driver from Ben Gardner <bgardner@wabtec.com>
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2. See the file COPYING for more details.
 */

#include <init.h>
#include "../w1.h"

#define W1_F2D_EEPROM_SIZE	128
#define W1_F2D_PAGE_COUNT	4
#define W1_F2D_PAGE_BITS	5
#define W1_F2D_PAGE_SIZE	(1 << W1_F2D_PAGE_BITS)
#define W1_F2D_PAGE_MASK	0x1F

#define W1_F2D_SCRATCH_BITS	3
#define W1_F2D_SCRATCH_SIZE	(1 << W1_F2D_SCRATCH_BITS)
#define W1_F2D_SCRATCH_MASK 	(W1_F2D_SCRATCH_SIZE - 1)

#define W1_F2D_READ_EEPROM	0xF0
#define W1_F2D_WRITE_SCRATCH	0x0F
#define W1_F2D_READ_SCRATCH	0xAA
#define W1_F2D_COPY_SCRATCH	0x55


#define W1_F2D_TPROG_MS		11

#define W1_F2D_READ_RETRIES	10
#define W1_F2D_READ_MAXLEN	8

#define DRIVERNAME	"ds2431"

static int ds2431_count = 0;

/*
 * Check the file size bounds and adjusts count as needed.
 * This would not be needed if the file size didn't reset to 0 after a write.
 */
static inline size_t ds2431_fix_count(loff_t off, size_t count, size_t size)
{
	if (off > size)
		return 0;

	if ((off + count) > size)
		return size - off;

	return count;
}

/*
 * Read a block from W1 ROM two times and compares the results.
 * If they are equal they are returned, otherwise the read
 * is repeated W1_F2D_READ_RETRIES times.
 *
 * count must not exceed W1_F2D_READ_MAXLEN.
 */
int ds2431_readblock(struct w1_device *dev, int off, int count, char *buf)
{
	struct w1_bus *bus = dev->bus;
	u8 wrbuf[3];
	u8 cmp[W1_F2D_READ_MAXLEN];
	int tries = W1_F2D_READ_RETRIES;

	do {
		wrbuf[0] = W1_F2D_READ_EEPROM;
		wrbuf[1] = off & 0xff;
		wrbuf[2] = off >> 8;

		if (w1_reset_select_slave(dev))
			return -1;

		w1_write_block(bus, wrbuf, 3);
		w1_read_block(bus, buf, count);

		if (w1_reset_select_slave(dev))
			return -1;

		w1_write_block(bus, wrbuf, 3);
		w1_read_block(bus, cmp, count);

		if (!memcmp(cmp, buf, count))
			return 0;
	} while (--tries);

	dev_err(&dev->dev, "proof reading failed %d times\n",
			W1_F2D_READ_RETRIES);

	return -1;
}

static ssize_t ds2431_cdev_read(struct cdev *cdev, void *buf, size_t count,
		loff_t off, ulong flags)
{
	struct w1_device *dev = cdev->priv;
	int todo = count;

	count = ds2431_fix_count(off, count, W1_F2D_EEPROM_SIZE);
	if (count == 0)
		return 0;

	/* read directly from the EEPROM in chunks of W1_F2D_READ_MAXLEN */
	while (todo > 0) {
		int block_read;

		if (todo >= W1_F2D_READ_MAXLEN)
			block_read = W1_F2D_READ_MAXLEN;
		else
			block_read = todo;

		if (ds2431_readblock(dev, off, block_read, buf) < 0)
			count = -EIO;

		todo -= W1_F2D_READ_MAXLEN;
		buf += W1_F2D_READ_MAXLEN;
		off += W1_F2D_READ_MAXLEN;
	}

	return count;
}

#ifdef CONFIG_W1_SLAVE_DS2431_WRITE
/*
 * Writes to the scratchpad and reads it back for verification.
 * Then copies the scratchpad to EEPROM.
 * The data must be aligned at W1_F2D_SCRATCH_SIZE bytes and
 * must be W1_F2D_SCRATCH_SIZE bytes long.
 * The master must be locked.
 *
 * @param sl	The slave structure
 * @param addr	Address for the write
 * @param len   length must be <= (W1_F2D_PAGE_SIZE - (addr & W1_F2D_PAGE_MASK))
 * @param data	The data to write
 * @return	0=Success -1=failure
 */
static int ds2431_write(struct w1_device *dev, int addr, int len, const u8 *data)
{
	struct w1_bus *bus = dev->bus;
	int tries = W1_F2D_READ_RETRIES;
	u8 wrbuf[4];
	u8 rdbuf[W1_F2D_SCRATCH_SIZE + 3];
	u8 es = (addr + len - 1) % W1_F2D_SCRATCH_SIZE;

retry:

	/* Write the data to the scratchpad */
	if (w1_reset_select_slave(dev))
		return -1;

	wrbuf[0] = W1_F2D_WRITE_SCRATCH;
	wrbuf[1] = addr & 0xff;
	wrbuf[2] = addr >> 8;

	w1_write_block(bus, wrbuf, 3);
	w1_write_block(bus, data, len);

	/* Read the scratchpad and verify */
	if (w1_reset_select_slave(dev))
		return -1;

	w1_write_8(bus, W1_F2D_READ_SCRATCH);
	w1_read_block(bus, rdbuf, len + 3);

	/* Compare what was read against the data written */
	if ((rdbuf[0] != wrbuf[1]) || (rdbuf[1] != wrbuf[2]) ||
	    (rdbuf[2] != es) || (memcmp(data, &rdbuf[3], len) != 0)) {

		if (--tries)
			goto retry;

		dev_err(&dev->dev,
			"could not write to eeprom, scratchpad compare failed %d times\n",
			W1_F2D_READ_RETRIES);

		return -1;
	}

	/* Copy the scratchpad to EEPROM */
	if (w1_reset_select_slave(dev))
		return -1;

	wrbuf[0] = W1_F2D_COPY_SCRATCH;
	wrbuf[3] = es;
	w1_write_block(bus, wrbuf, 4);

	/* Sleep for tprog ms to wait for the write to complete */
	mdelay(W1_F2D_TPROG_MS);

	/* Reset the bus to wake up the EEPROM  */
	w1_reset_bus(bus);

	return 0;
}

static ssize_t ds2431_cdev_write(struct cdev *cdev, const void *buf, size_t count,
		loff_t off, ulong flags)
{
	struct w1_device *dev = cdev->priv;
	int addr, len;
	int copy;

	count = ds2431_fix_count(off, count, W1_F2D_EEPROM_SIZE);
	if (count == 0)
		return 0;

	/* Can only write data in blocks of the size of the scratchpad */
	addr = off;
	len = count;
	while (len > 0) {

		/* if len too short or addr not aligned */
		if (len < W1_F2D_SCRATCH_SIZE || addr & W1_F2D_SCRATCH_MASK) {
			char tmp[W1_F2D_SCRATCH_SIZE];

			/* read the block and update the parts to be written */
			if (ds2431_readblock(dev, addr & ~W1_F2D_SCRATCH_MASK,
					W1_F2D_SCRATCH_SIZE, tmp)) {
				count = -EIO;
				goto out_up;
			}

			/* copy at most to the boundary of the PAGE or len */
			copy = W1_F2D_SCRATCH_SIZE -
				(addr & W1_F2D_SCRATCH_MASK);

			if (copy > len)
				copy = len;

			memcpy(&tmp[addr & W1_F2D_SCRATCH_MASK], buf, copy);
			if (ds2431_write(dev, addr & ~W1_F2D_SCRATCH_MASK,
					W1_F2D_SCRATCH_SIZE, tmp) < 0) {
				count = -EIO;
				goto out_up;
			}
		} else {

			copy = W1_F2D_SCRATCH_SIZE;
			if (ds2431_write(dev, addr, copy, buf) < 0) {
				count = -EIO;
				goto out_up;
			}
		}
		buf += copy;
		addr += copy;
		len -= copy;
	}

out_up:
	return count;
}
#else
#define ds2431_cdev_write NULL
#endif

static struct cdev_operations ds2431_ops = {
	.read	= ds2431_cdev_read,
	.write	= ds2431_cdev_write,
	.lseek	= dev_lseek_default,
};

static int ds2431_probe(struct w1_device *dev)
{
	struct cdev *cdev;

	cdev = xzalloc(sizeof(*cdev));
	cdev->dev	= &dev->dev;
	cdev->priv	= dev;
	cdev->ops	= &ds2431_ops;
	cdev->size	= W1_F2D_EEPROM_SIZE;
	cdev->name	= basprintf(DRIVERNAME"%d", ds2431_count++);
	if (cdev->name == NULL)
		return -ENOMEM;

	return devfs_create(cdev);
}

struct w1_driver ds2431_driver = {
	.drv = {
		.name = DRIVERNAME,
	},
	.probe		= ds2431_probe,
	.fid		= 0x2d,
};

static int w1_ds2431_init(void)
{
	return w1_driver_register(&ds2431_driver);
}
device_initcall(w1_ds2431_init);
