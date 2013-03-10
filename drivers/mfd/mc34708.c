/*
 * Copyright (C) 2007 Sascha Hauer, Pengutronix
 *               2009 Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <xfuncs.h>
#include <errno.h>
#include <spi/spi.h>
#include <malloc.h>

#include <i2c/i2c.h>
#include <mfd/mc34708.h>

#define DRIVERNAME		"mc34708"

#define to_mc34708(a)		container_of(a, struct mc34708, cdev)

static struct mc34708 *mc_dev;

struct mc34708 *mc34708_get(void)
{
	if (!mc_dev)
		return NULL;

	return mc_dev;
}
EXPORT_SYMBOL(mc34708_get);

#ifdef CONFIG_SPI
static int spi_rw(struct spi_device *spi, void * buf, size_t len)
{
	int ret;

	struct spi_transfer t = {
		.tx_buf = (const void *)buf,
		.rx_buf = buf,
		.len = len,
		.cs_change = 0,
		.delay_usecs = 0,
	};
	struct spi_message m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);

	if ((ret = spi_sync(spi, &m)))
		return ret;
	return 0;
}

#define MXC_PMIC_REG_NUM(reg)	(((reg) & 0x3f) << 25)
#define MXC_PMIC_WRITE		(1 << 31)

static int mc34708_spi_reg_read(struct mc34708 *mc34708, enum mc34708_reg reg, u32 *val)
{
	uint32_t buf;

	buf = MXC_PMIC_REG_NUM(reg);

	spi_rw(mc34708->spi, &buf, 4);

	*val = buf;

	return 0;
}

static int mc34708_spi_reg_write(struct mc34708 *mc34708, enum mc34708_reg reg, u32 val)
{
	uint32_t buf = MXC_PMIC_REG_NUM(reg) | MXC_PMIC_WRITE | (val & 0xffffff);

	spi_rw(mc34708->spi, &buf, 4);

	return 0;
}
#endif

#ifdef CONFIG_I2C
static int mc34708_i2c_reg_read(struct mc34708 *mc34708, enum mc34708_reg reg, u32 *val)
{
	u8 buf[3];
	int ret;

	ret = i2c_read_reg(mc34708->client, reg, buf, 3);
	*val = buf[0] << 16 | buf[1] << 8 | buf[2] << 0;

	return ret == 3 ? 0 : ret;
}

static int mc34708_i2c_reg_write(struct mc34708 *mc34708, enum mc34708_reg reg, u32 val)
{
	u8 buf[] = {
		val >> 16,
		val >>  8,
		val >>  0,
	};
	int ret;

	ret = i2c_write_reg(mc34708->client, reg, buf, 3);

	return ret == 3 ? 0 : ret;
}
#endif

int mc34708_reg_write(struct mc34708 *mc34708, enum mc34708_reg reg, u32 val)
{
#ifdef CONFIG_I2C
	if (mc34708->mode == MC34708_MODE_I2C)
		return mc34708_i2c_reg_write(mc34708, reg, val);
#endif
#ifdef CONFIG_SPI
	if (mc34708->mode == MC34708_MODE_SPI)
		return mc34708_spi_reg_write(mc34708, reg, val);
#endif
	return -EINVAL;
}
EXPORT_SYMBOL(mc34708_reg_write);

int mc34708_reg_read(struct mc34708 *mc34708, enum mc34708_reg reg, u32 *val)
{
#ifdef CONFIG_I2C
	if (mc34708->mode == MC34708_MODE_I2C)
		return mc34708_i2c_reg_read(mc34708, reg, val);
#endif
#ifdef CONFIG_SPI
	if (mc34708->mode == MC34708_MODE_SPI)
		return mc34708_spi_reg_read(mc34708, reg, val);
#endif
	return -EINVAL;
}
EXPORT_SYMBOL(mc34708_reg_read);

int mc34708_set_bits(struct mc34708 *mc34708, enum mc34708_reg reg, u32 mask, u32 val)
{
	u32 tmp;
	int err;

	err = mc34708_reg_read(mc34708, reg, &tmp);
	tmp = (tmp & ~mask) | val;

	if (!err)
		err = mc34708_reg_write(mc34708, reg, tmp);

	return err;
}
EXPORT_SYMBOL(mc34708_set_bits);

static ssize_t mc_read(struct cdev *cdev, void *_buf, size_t count,
		loff_t offset, ulong flags)
{
	struct mc34708 *priv = to_mc34708(cdev);
	u32 *buf = _buf;
	size_t i = count >> 2;
	int err;

	offset >>= 2;

	while (i) {
		err = mc34708_reg_read(priv, offset, buf);
		if (err)
			return (ssize_t)err;
		buf++;
		i--;
		offset++;
	}

	return count;
}

static ssize_t mc_write(struct cdev *cdev, const void *_buf, size_t count,
		loff_t offset, ulong flags)
{
	struct mc34708 *mc34708 = to_mc34708(cdev);
	const u32 *buf = _buf;
	size_t i = count >> 2;
	int err;

	offset >>= 2;

	while (i) {
		err = mc34708_reg_write(mc34708, offset, *buf);
		if (err)
			return (ssize_t)err;
		buf++;
		i--;
		offset++;
	}

	return count;
}

static struct file_operations mc_fops = {
	.lseek	= dev_lseek_default,
	.read	= mc_read,
	.write	= mc_write,
};

static int mc34708_query_revision(struct mc34708 *mc34708)
{
	unsigned int rev_id;
	int rev;

	mc34708_reg_read(mc34708, 7, &rev_id);

	if (rev_id > 0xFFF)
		return -EINVAL;

	rev = rev_id & 0xFFF;

	dev_info(mc_dev->cdev.dev, "MC34708 ID: 0x%04x\n", rev);

	mc34708->revision = rev;

	return rev;
}

static int mc_probe(struct device_d *dev, enum mc34708_mode mode)
{
	int rev;

	if (mc_dev)
		return -EBUSY;

	mc_dev = xzalloc(sizeof(struct mc34708));
	mc_dev->mode = mode;
	mc_dev->cdev.name = DRIVERNAME;
	if (mode == MC34708_MODE_I2C) {
		mc_dev->client = to_i2c_client(dev);
	}
	if (mode == MC34708_MODE_SPI) {
		mc_dev->spi = dev->type_data;
		mc_dev->spi->mode = SPI_MODE_0 | SPI_CS_HIGH;
		mc_dev->spi->bits_per_word = 32;
	}
	mc_dev->cdev.size = 256;
	mc_dev->cdev.dev = dev;
	mc_dev->cdev.ops = &mc_fops;

	rev = mc34708_query_revision(mc_dev);
	if (rev < 0) {
		free(mc_dev);
		mc_dev = NULL;
		return -EINVAL;
	}

	devfs_create(&mc_dev->cdev);

	return 0;
}

#ifdef CONFIG_I2C
static int mc_i2c_probe(struct device_d *dev)
{
	return mc_probe(dev, MC34708_MODE_I2C);
}

static struct driver_d mc_i2c_driver = {
	.name  = "mc34708-i2c",
	.probe = mc_i2c_probe,
};

static int mc_i2c_init(void)
{
	return i2c_driver_register(&mc_i2c_driver);
}

device_initcall(mc_i2c_init);
#endif

#ifdef CONFIG_SPI
static int mc_spi_probe(struct device_d *dev)
{
	return mc_probe(dev, MC34708_MODE_SPI);
}

static struct driver_d mc_spi_driver = {
	.name  = "mc34708-spi",
	.probe = mc_spi_probe,
};
device_spi_driver(mc_spi_driver);
#endif
