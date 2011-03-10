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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
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
#include <mfd/mc13892.h>

#define DRIVERNAME		"mc13892"

#define to_mc13892(a)		container_of(a, struct mc13892, cdev)

static struct mc13892 *mc_dev;

struct mc13892 *mc13892_get(void)
{
	if (!mc_dev)
		return NULL;

	return mc_dev;
}
EXPORT_SYMBOL(mc13892_get);

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

static int mc13892_spi_reg_read(struct mc13892 *mc13892, enum mc13892_reg reg, u32 *val)
{
	uint32_t buf;

	buf = MXC_PMIC_REG_NUM(reg);

	spi_rw(mc13892->spi, &buf, 4);

	*val = buf;

	return 0;
}

static int mc13892_spi_reg_write(struct mc13892 *mc13892, enum mc13892_reg reg, u32 val)
{
	uint32_t buf = MXC_PMIC_REG_NUM(reg) | MXC_PMIC_WRITE | (val & 0xffffff);

	spi_rw(mc13892->spi, &buf, 4);

	return 0;
}
#endif

#ifdef CONFIG_I2C
static int mc13892_i2c_reg_read(struct mc13892 *mc13892, enum mc13892_reg reg, u32 *val)
{
	u8 buf[3];
	int ret;

	ret = i2c_read_reg(mc13892->client, reg, buf, 3);
	*val = buf[0] << 16 | buf[1] << 8 | buf[2] << 0;

	return ret == 3 ? 0 : ret;
}

static int mc13892_i2c_reg_write(struct mc13892 *mc13892, enum mc13892_reg reg, u32 val)
{
	u8 buf[] = {
		val >> 16,
		val >>  8,
		val >>  0,
	};
	int ret;

	ret = i2c_write_reg(mc13892->client, reg, buf, 3);

	return ret == 3 ? 0 : ret;
}
#endif

int mc13892_reg_write(struct mc13892 *mc13892, enum mc13892_reg reg, u32 val)
{
#ifdef CONFIG_I2C
	if (mc13892->mode == MC13892_MODE_I2C)
		return mc13892_i2c_reg_write(mc13892, reg, val);
#endif
#ifdef CONFIG_SPI
	if (mc13892->mode == MC13892_MODE_SPI)
		return mc13892_spi_reg_write(mc13892, reg, val);
#endif
	return -EINVAL;
}
EXPORT_SYMBOL(mc13892_reg_write);

int mc13892_reg_read(struct mc13892 *mc13892, enum mc13892_reg reg, u32 *val)
{
#ifdef CONFIG_I2C
	if (mc13892->mode == MC13892_MODE_I2C)
		return mc13892_i2c_reg_read(mc13892, reg, val);
#endif
#ifdef CONFIG_SPI
	if (mc13892->mode == MC13892_MODE_SPI)
		return mc13892_spi_reg_read(mc13892, reg, val);
#endif
	return -EINVAL;
}
EXPORT_SYMBOL(mc13892_reg_read);

int mc13892_set_bits(struct mc13892 *mc13892, enum mc13892_reg reg, u32 mask, u32 val)
{
	u32 tmp;
	int err;

	err = mc13892_reg_read(mc13892, reg, &tmp);
	tmp = (tmp & ~mask) | val;

	if (!err)
		err = mc13892_reg_write(mc13892, reg, tmp);

	return err;
}
EXPORT_SYMBOL(mc13892_set_bits);

static ssize_t mc_read(struct cdev *cdev, void *_buf, size_t count, ulong offset, ulong flags)
{
	struct mc13892 *priv = to_mc13892(cdev);
	u32 *buf = _buf;
	size_t i = count >> 2;
	int err;

	offset >>= 2;

	while (i) {
		err = mc13892_reg_read(priv, offset, buf);
		if (err)
			return (ssize_t)err;
		buf++;
		i--;
		offset++;
	}

	return count;
}

static ssize_t mc_write(struct cdev *cdev, const void *_buf, size_t count, ulong offset, ulong flags)
{
	struct mc13892 *mc13892 = to_mc13892(cdev);
	const u32 *buf = _buf;
	size_t i = count >> 2;
	int err;

	offset >>= 2;

	while (i) {
		err = mc13892_reg_write(mc13892, offset, *buf);
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

struct mc13892_rev {
	u16 rev_id;
	enum mc13892_revision rev;
	char *revstr;
};

static struct mc13892_rev mc13892_revisions[] = {
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

static int mc13893_query_revision(struct mc13892 *mc13892)
{
	unsigned int rev_id;
	char *revstr;
	int rev, i;

	mc13892_reg_read(mc13892, 7, &rev_id);

	for (i = 0; i < ARRAY_SIZE(mc13892_revisions); i++)
		if ((rev_id & 0x1f) == mc13892_revisions[i].rev_id)
			break;

	if (i == ARRAY_SIZE(mc13892_revisions))
		return -EINVAL;

	rev = mc13892_revisions[i].rev;
	revstr = mc13892_revisions[i].revstr;

	if (rev == MC13892_REVISION_2_0) {
		if ((rev_id >> 9) & 0x3) {
			rev = MC13892_REVISION_2_0a;
			revstr = "2.0a";
		}
	}

	dev_info(mc_dev->cdev.dev, "PMIC ID: 0x%08x [Rev: %s]\n",
			rev_id, revstr);

	mc13892->revision = rev;

	return rev;
}

static int mc_probe(struct device_d *dev, enum mc13892_mode mode)
{
	int rev;

	if (mc_dev)
		return -EBUSY;

	mc_dev = xzalloc(sizeof(struct mc13892));
	mc_dev->mode = mode;
	mc_dev->cdev.name = DRIVERNAME;
	if (mode == MC13892_MODE_I2C) {
		mc_dev->client = to_i2c_client(dev);
	}
	if (mode == MC13892_MODE_SPI) {
		mc_dev->spi = dev->type_data;
		mc_dev->spi->mode = SPI_MODE_0 | SPI_CS_HIGH;
		mc_dev->spi->bits_per_word = 32;
	}
	mc_dev->cdev.size = 256;
	mc_dev->cdev.dev = dev;
	mc_dev->cdev.ops = &mc_fops;

	rev = mc13893_query_revision(mc_dev);
	if (rev < 0) {
		free(mc_dev);
		return -EINVAL;
	}

	devfs_create(&mc_dev->cdev);

	return 0;
}

static int mc_i2c_probe(struct device_d *dev)
{
	return mc_probe(dev, MC13892_MODE_I2C);
}

static int mc_spi_probe(struct device_d *dev)
{
	return mc_probe(dev, MC13892_MODE_SPI);
}

static struct driver_d mc_i2c_driver = {
	.name  = "mc13892-i2c",
	.probe = mc_i2c_probe,
};

static struct driver_d mc_spi_driver = {
	.name  = "mc13892-spi",
	.probe = mc_spi_probe,
};

static int mc_init(void)
{
        register_driver(&mc_i2c_driver);
        register_driver(&mc_spi_driver);
        return 0;
}

device_initcall(mc_init);
