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
#include <malloc.h>

#include <i2c/i2c.h>
#include <spi/spi.h>
#include <mfd/mc13xxx.h>

#define DRIVERNAME		"mc13xxx"

struct mc13xxx {
	struct cdev			cdev;
	union {
		struct i2c_client	*client;
		struct spi_device	*spi;
	};
	int				revision;
	int				(*reg_read)(struct mc13xxx*, u8, u32*);
	int				(*reg_write)(struct mc13xxx*, u8, u32);
};

struct mc13xxx_devtype {
	int	(*revision)(struct mc13xxx*);
};

#define to_mc13xxx(a)		container_of(a, struct mc13xxx, cdev)

static struct mc13xxx *mc_dev;

struct mc13xxx *mc13xxx_get(void)
{
	return mc_dev;
}
EXPORT_SYMBOL(mc13xxx_get);

int mc13xxx_revision(struct mc13xxx *mc13xxx)
{
	return mc13xxx->revision;
}
EXPORT_SYMBOL(mc13xxx_revision);

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

static int mc13xxx_spi_reg_read(struct mc13xxx *mc13xxx, u8 reg, u32 *val)
{
	uint32_t buf;

	buf = MXC_PMIC_REG_NUM(reg);

	spi_rw(mc13xxx->spi, &buf, 4);

	*val = buf;

	return 0;
}

static int mc13xxx_spi_reg_write(struct mc13xxx *mc13xxx, u8 reg, u32 val)
{
	uint32_t buf = MXC_PMIC_REG_NUM(reg) | MXC_PMIC_WRITE | (val & 0xffffff);

	spi_rw(mc13xxx->spi, &buf, 4);

	return 0;
}
#endif

#ifdef CONFIG_I2C
static int mc13xxx_i2c_reg_read(struct mc13xxx *mc13xxx, u8 reg, u32 *val)
{
	u8 buf[3];
	int ret;

	ret = i2c_read_reg(mc13xxx->client, reg, buf, 3);
	*val = buf[0] << 16 | buf[1] << 8 | buf[2] << 0;

	return ret == 3 ? 0 : ret;
}

static int mc13xxx_i2c_reg_write(struct mc13xxx *mc13xxx, u8 reg, u32 val)
{
	u8 buf[] = {
		val >> 16,
		val >>  8,
		val >>  0,
	};
	int ret;

	ret = i2c_write_reg(mc13xxx->client, reg, buf, 3);

	return ret == 3 ? 0 : ret;
}
#endif

int mc13xxx_reg_write(struct mc13xxx *mc13xxx, u8 reg, u32 val)
{
	return mc13xxx->reg_write(mc13xxx, reg, val);
}
EXPORT_SYMBOL(mc13xxx_reg_write);

int mc13xxx_reg_read(struct mc13xxx *mc13xxx, u8 reg, u32 *val)
{
	return mc13xxx->reg_read(mc13xxx, reg, val);
}
EXPORT_SYMBOL(mc13xxx_reg_read);

int mc13xxx_set_bits(struct mc13xxx *mc13xxx, u8 reg, u32 mask, u32 val)
{
	u32 tmp;
	int err;

	err = mc13xxx_reg_read(mc13xxx, reg, &tmp);
	tmp = (tmp & ~mask) | val;

	if (!err)
		err = mc13xxx_reg_write(mc13xxx, reg, tmp);

	return err;
}
EXPORT_SYMBOL(mc13xxx_set_bits);

static ssize_t mc_read(struct cdev *cdev, void *_buf, size_t count, loff_t offset, ulong flags)
{
	struct mc13xxx *mc13xxx = to_mc13xxx(cdev);
	u32 *buf = _buf;
	size_t i = count >> 2;
	int err;

	offset >>= 2;

	while (i) {
		err = mc13xxx_reg_read(mc13xxx, offset, buf);
		if (err)
			return (ssize_t)err;
		buf++;
		i--;
		offset++;
	}

	return count;
}

static ssize_t mc_write(struct cdev *cdev, const void *_buf, size_t count, loff_t offset, ulong flags)
{
	struct mc13xxx *mc13xxx = to_mc13xxx(cdev);
	const u32 *buf = _buf;
	size_t i = count >> 2;
	int err;

	offset >>= 2;

	while (i) {
		err = mc13xxx_reg_write(mc13xxx, offset, *buf);
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

static int __init mc13783_revision(struct mc13xxx *mc13xxx)
{
	unsigned int rev_id;
	char revstr[5];
	int rev;

	mc13xxx_reg_read(mc13xxx, MC13XXX_REG_IDENTIFICATION, &rev_id);

	if (((rev_id >> 6) & 0x7) != 0x2)
		return -ENODEV;


	rev = (((rev_id & 0x18) >> 3) << 4) | (rev_id & 0x7);
	/* Ver 0.2 is actually 3.2a. Report as 3.2 */
	if (rev == 0x02) {
		rev = 0x32;
		strcpy(revstr, "3.2a");
	} else
		sprintf(revstr, "%d.%d", rev / 0x10, rev % 10);

	dev_info(mc_dev->cdev.dev, "Found MC13783 ID: 0x%06x [Rev: %s]\n",
		 rev_id, revstr);

	return rev;
}

static struct __init {
	u16	rev_id;
	int	rev;
	char	*revstr;
} mc13892_revisions[] = {
	{ 0x01, MC13892_REVISION_1_0, "1.0" },
	{ 0x09, MC13892_REVISION_1_1, "1.1" },
	{ 0x0a, MC13892_REVISION_1_2, "1.2" },
	{ 0x10, MC13892_REVISION_2_0, "2.0" },
	{ 0x11, MC13892_REVISION_2_1, "2.1" },
	{ 0x18, MC13892_REVISION_3_0, "3.0" },
	{ 0x19, MC13892_REVISION_3_1, "3.1" },
	{ 0x1a, MC13892_REVISION_3_2, "3.2" },
	{ 0x02, MC13892_REVISION_3_2a, "3.2a" },
	{ 0x1b, MC13892_REVISION_3_3, "3.3" },
	{ 0x1d, MC13892_REVISION_3_5, "3.5" },
};

static int __init mc13892_revision(struct mc13xxx *mc13xxx)
{
	unsigned int rev_id;
	char revstr[5];
	int rev, i;

	mc13xxx_reg_read(mc13xxx, MC13XXX_REG_IDENTIFICATION, &rev_id);

	if (((rev_id >> 6) & 0x7) != 0x7)
		return -ENODEV;

	for (i = 0; i < ARRAY_SIZE(mc13892_revisions); i++)
		if ((rev_id & 0x1f) == mc13892_revisions[i].rev_id)
			break;

	if (i == ARRAY_SIZE(mc13892_revisions))
		return -ENOSYS;

	rev = mc13892_revisions[i].rev;
	strcpy(revstr, mc13892_revisions[i].revstr);

	if (rev == MC13892_REVISION_2_0) {
		if ((rev_id >> 9) & 0x3) {
			rev = MC13892_REVISION_2_0a;
			strcpy(revstr, "2.0a");
		}
	}

	dev_info(mc_dev->cdev.dev, "Found MC13892 ID: 0x%06x [Rev: %s]\n",
		 rev_id, revstr);

	return rev;
}

static int __init mc34708_revision(struct mc13xxx *mc13xxx)
{
	unsigned int rev_id;

	mc13xxx_reg_read(mc13xxx, MC13XXX_REG_IDENTIFICATION, &rev_id);

	if (rev_id > 0xfff)
		return -ENODEV;

	dev_info(mc_dev->cdev.dev, "Found MC34708 ID: 0x%03x\n", rev_id);

	return (int)rev_id;
}

static int __init mc13xxx_probe(struct device_d *dev)
{
	struct mc13xxx_devtype *devtype;
	int ret, rev;

	if (mc_dev)
		return -EBUSY;

	ret = dev_get_drvdata(dev, (unsigned long *)&devtype);
	if (ret)
		return ret;

	mc_dev = xzalloc(sizeof(*mc_dev));
	mc_dev->cdev.name = DRIVERNAME;

#ifdef CONFIG_I2C
	if (dev->bus == &i2c_bus) {
		mc_dev->client = to_i2c_client(dev);
		mc_dev->reg_read = mc13xxx_i2c_reg_read;
		mc_dev->reg_write = mc13xxx_i2c_reg_write;
	}
#endif
#ifdef CONFIG_SPI
	if (dev->bus == &spi_bus) {
		mc_dev->spi = dev->type_data;
		mc_dev->spi->mode = SPI_MODE_0 | SPI_CS_HIGH;
		mc_dev->spi->bits_per_word = 32;
		mc_dev->spi->max_speed_hz = 20000000;
		mc_dev->reg_read = mc13xxx_spi_reg_read;
		mc_dev->reg_write = mc13xxx_spi_reg_write;
	}
#endif

	mc_dev->cdev.size = 256;
	mc_dev->cdev.dev = dev;
	mc_dev->cdev.ops = &mc_fops;

	rev = devtype->revision(mc_dev);
	if (rev < 0) {
		dev_err(mc_dev->cdev.dev, "No PMIC detected.\n");
		free(mc_dev);
		mc_dev = NULL;
		return rev;
	}

	mc_dev->revision = rev;
	devfs_create(&mc_dev->cdev);

	return 0;
}

static struct mc13xxx_devtype mc13783_devtype = {
	.revision	= mc13783_revision,
};

static struct mc13xxx_devtype mc13892_devtype = {
	.revision	= mc13892_revision,
};

static struct mc13xxx_devtype mc34708_devtype = {
	.revision	= mc34708_revision,
};

static struct platform_device_id mc13xxx_ids[] = {
	{ .name = "mc13783", .driver_data = (ulong)&mc13783_devtype, },
	{ .name = "mc13892", .driver_data = (ulong)&mc13892_devtype, },
	{ .name = "mc34708", .driver_data = (ulong)&mc34708_devtype, },
	{ }
};

static __maybe_unused struct of_device_id mc13xxx_dt_ids[] = {
	{ .compatible = "fsl,mc13783", .data = (ulong)&mc13783_devtype, },
	{ .compatible = "fsl,mc13892", .data = (ulong)&mc13892_devtype, },
	{ .compatible = "fsl,mc34708", .data = (ulong)&mc34708_devtype, },
	{ }
};

#ifdef CONFIG_I2C
static struct driver_d mc13xxx_i2c_driver = {
	.name		= "mc13xxx-i2c",
	.probe		= mc13xxx_probe,
	.id_table	= mc13xxx_ids,
	.of_compatible	= DRV_OF_COMPAT(mc13xxx_dt_ids),
};

static int __init mc13xxx_i2c_init(void)
{
	return i2c_driver_register(&mc13xxx_i2c_driver);
}
device_initcall(mc13xxx_i2c_init);
#endif

#ifdef CONFIG_SPI
static struct driver_d mc13xxx_spi_driver = {
	.name		= "mc13xxx-spi",
	.probe		= mc13xxx_probe,
	.id_table	= mc13xxx_ids,
	.of_compatible	= DRV_OF_COMPAT(mc13xxx_dt_ids),
};
device_spi_driver(mc13xxx_spi_driver);
#endif
