// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2012 Jan Luebbe, Pengutronix
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <spi/spi.h>
#include <errno.h>

#define REG_ID0			0x00
#define REG_ID1			0x01

#define REG_GC00		0x02
#define REG_GC01		0x03
#define REG_GC02		0x04
#define REG_GC03		0x05
#define REG_GC04		0x06
#define REG_GC05		0x07
#define REG_GC06		0x08
#define REG_GC07		0x09
#define REG_GC08		0x0a
#define REG_GC09		0x0b
#define REG_GC10		0x0c
#define REG_GC11		0x0d

#define REG_PSTAT1(p)		(0x10 * p + 0xe)
#define REG_PSTAT2(p)		(0x10 * p + 0xf)

#define CMD_WRITE		0x02
#define CMD_READ		0x03

struct micrel_switch_priv {
	struct cdev             cdev;
	struct spi_device       *spi;
	unsigned int		p_enable;
};

static int micrel_switch_read_reg(struct spi_device *spi, uint8_t reg)
{
	uint8_t tx[2];
	uint8_t rx[1];
	int ret;

	tx[0] = CMD_READ;
	tx[1] = reg;

	ret = spi_write_then_read(spi, tx, 2, rx, 1);
	if (ret < 0)
		return ret;

	return rx[0];
}

static void micrel_switch_write_reg(struct spi_device *spi, uint8_t reg, uint8_t val)
{
	uint8_t tx[3];

	tx[0] = CMD_WRITE;
	tx[1] = reg;
	tx[2] = val;

	spi_write_then_read(spi, tx, 3, NULL, 0);
}

static int micrel_switch_enable_set(struct param_d *param, void *_priv)
{
	struct micrel_switch_priv *priv = _priv;
	struct spi_device *spi = priv->spi;

	if (priv->p_enable)
		micrel_switch_write_reg(spi, REG_ID1, 1);
	else
		micrel_switch_write_reg(spi, REG_ID1, 0);

	return 0;
}

static ssize_t micel_switch_read(struct cdev *cdev, void *_buf, size_t count, loff_t offset, ulong flags)
{
	int i, ret;
	uint8_t *buf = _buf;
	struct micrel_switch_priv *priv = cdev->priv;

	for (i = 0; i < count; i++) {
		ret = micrel_switch_read_reg(priv->spi, offset);
		if (ret < 0)
			return ret;
		*buf = ret;
		buf++;
		offset++;
	}

	return count;
}

static ssize_t micel_switch_write(struct cdev *cdev, const void *_buf, size_t count, loff_t offset, ulong flags)
{
	int i;
	const uint8_t *buf = _buf;
	struct micrel_switch_priv *priv = cdev->priv;

	for (i = 0; i < count; i++) {
		micrel_switch_write_reg(priv->spi, offset, *buf);
		buf++;
		offset++;
	}

	return count;
}

static struct cdev_operations micrel_switch_ops = {
	.read  = micel_switch_read,
	.write = micel_switch_write,
	.lseek = dev_lseek_default,
};

static int micrel_switch_probe(struct device_d *dev)
{
	struct micrel_switch_priv *priv;
	int ret = 0;

	priv = xzalloc(sizeof(*priv));

	dev->priv = priv;

	priv->spi = (struct spi_device *)dev->type_data;
	priv->spi->mode = SPI_MODE_0;
	priv->spi->bits_per_word = 8;

	ret = micrel_switch_read_reg(priv->spi, REG_ID0);
	if (ret < 0) {
		dev_err(&priv->spi->dev, "failed to read device id\n");
		return ret;
	}
	if (ret != 0x95) {
		dev_err(&priv->spi->dev, "unknown device id: %02x\n", ret);
		return -ENODEV;
	}

	priv->cdev.name = basprintf("switch%d", dev->id);
	priv->cdev.size = 256;
	priv->cdev.ops = &micrel_switch_ops;
	priv->cdev.priv = priv;
	priv->cdev.dev = dev;
	devfs_create(&priv->cdev);

	dev_add_param_bool(dev, "enable", micrel_switch_enable_set,
			NULL, &priv->p_enable, priv);

	priv->p_enable = 1;
	micrel_switch_write_reg(priv->spi, REG_ID1, 1);

	return 0;
}

static struct driver_d micrel_switch_driver = {
	.name  = "ksz8864rmn",
	.probe = micrel_switch_probe,
};
device_spi_driver(micrel_switch_driver);
