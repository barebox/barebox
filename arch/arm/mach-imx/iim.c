/*
 * iim.c - i.MX IIM fusebox driver
 *
 * Provide an interface for programming and sensing the information that are
 * stored in on-chip fuse elements. This functionality is part of the IC
 * Identification Module (IIM), which is present on some i.MX CPUs.
 *
 * Copyright (c) 2010 Baruch Siach <baruch@tkos.co.il>,
 * 	Orex Computed Radiography
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <xfuncs.h>
#include <errno.h>
#include <param.h>
#include <fcntl.h>
#include <malloc.h>

#include <io.h>

#include <mach/iim.h>

#define DRIVERNAME	"imx_iim"

static unsigned long mac_addr_base;

struct iim_priv {
	struct cdev cdev;
	void __iomem *base;
	void __iomem *bankbase;
	int bank;
	int banksize;
};

static int do_fuse_sense(void __iomem *reg_base, unsigned int bank,
		unsigned int row)
{
	u8 err, stat;

	if (bank > 7) {
		printf("%s: invalid bank number\n", __func__);
		return -EINVAL;
	}

	if (row > 255) {
		printf("%s: invalid row index\n", __func__);
		return -EINVAL;
	}

	/* clear status and error registers */
	writeb(3, reg_base + IIM_STATM);
	writeb(0xfe, reg_base + IIM_ERR);

	/* upper and lower address halves */
	writeb((bank << 3) | (row >> 5), reg_base + IIM_UA);
	writeb((row << 3) & 0xf8, reg_base + IIM_LA);

	/* start fuse sensing */
	writeb(0x08, reg_base + IIM_FCTL);

	/* wait for sense done */
	while ((readb(reg_base + IIM_STAT) & 0x80) != 0)
		;

	stat = readb(reg_base + IIM_STAT);
	writeb(stat, reg_base + IIM_STAT);

	err = readb(reg_base + IIM_ERR);
	if (err) {
		printf("%s: sense error (0x%02x)\n", __func__, err);
		return -EIO;
	}

	return readb(reg_base + IIM_SDAT);
}

static ssize_t imx_iim_cdev_read(struct cdev *cdev, void *buf, size_t count,
		loff_t offset, ulong flags)
{
	ulong size, i;
	struct iim_priv *priv = cdev->priv;
	const char *sense_param;
	unsigned long explicit_sense = 0;

	if ((sense_param = dev_get_param(cdev->dev, "explicit_sense_enable")))
		explicit_sense = simple_strtoul(sense_param, NULL, 0);

	size = min((loff_t)count, priv->banksize - offset);
	if (explicit_sense) {
		for (i = 0; i < size; i++) {
			int row_val;

			row_val = do_fuse_sense(priv->base,
						priv->bank, offset + i);
			if (row_val < 0)
				return row_val;
			((u8 *)buf)[i] = (u8)row_val;
		}
	} else {
		for (i = 0; i < size; i++)
			((u8 *)buf)[i] = ((u8 *)priv->bankbase)[(offset+i)*4];
	}

	return size;
}

#ifdef CONFIG_IMX_IIM_FUSE_BLOW
static int do_fuse_blow(void __iomem *reg_base, unsigned int bank,
		unsigned int row, u8 value)
{
	int bit, ret = 0;
	u8 err, stat;

	if (bank > 7) {
		printf("%s: invalid bank number\n", __func__);
		return -EINVAL;
	}

	if (row > 255) {
		printf("%s: invalid row index\n", __func__);
		return -EINVAL;
	}

	/* clear status and error registers */
	writeb(3, reg_base + IIM_STATM);
	writeb(0xfe, reg_base + IIM_ERR);

	/* unprotect fuse programing */
	writeb(0xaa, reg_base + IIM_PREG_P);

	/* upper half address register */
	writeb((bank << 3) | (row >> 5), reg_base + IIM_UA);

	for (bit = 0; bit < 8; bit++) {
		if (((value >> bit) & 1) == 0)
			continue;

		/* lower half address register */
		writeb(((row << 3) | bit), reg_base + IIM_LA);

		/* start fuse programing */
		writeb(0x71, reg_base + IIM_FCTL);

		/* wait for program done */
		while ((readb(reg_base + IIM_STAT) & 0x80) != 0)
			;

		/* clear program done status */
		stat = readb(reg_base + IIM_STAT);
		writeb(stat, reg_base + IIM_STAT);

		err = readb(reg_base + IIM_ERR);
		if (err) {
			printf("%s: bank %u, row %u, bit %d program error "
					"(0x%02x)\n", __func__, bank, row, bit,
					err);
			ret = -EIO;
			goto out;
		}
	}

out:
	/* protect fuse programing */
	writeb(0, reg_base + IIM_PREG_P);
	return ret;
}
#endif /* CONFIG_IMX_IIM_FUSE_BLOW */

static ssize_t imx_iim_cdev_write(struct cdev *cdev, const void *buf, size_t count,
		loff_t offset, ulong flags)
{
	ulong size, i;
	struct iim_priv *priv = cdev->priv;
	const char *write_param;
	unsigned int blow_enable = 0;

	if ((write_param = dev_get_param(cdev->dev, "permanent_write_enable")))
		blow_enable = simple_strtoul(write_param, NULL, 0);

	size = min((loff_t)count, priv->banksize - offset);
#ifdef CONFIG_IMX_IIM_FUSE_BLOW
	if (blow_enable) {
		for (i = 0; i < size; i++) {
			int ret;

			ret = do_fuse_blow(priv->base, priv->bank,
					   offset + i, ((u8 *)buf)[i]);
			if (ret < 0)
				return ret;
		}
	} else
#endif /* CONFIG_IMX_IIM_FUSE_BLOW */
	{
		for (i = 0; i < size; i++)
			((u8 *)priv->bankbase)[(offset+i)*4] = ((u8 *)buf)[i];
	}

	return size;
}

static struct file_operations imx_iim_ops = {
	.read	= imx_iim_cdev_read,
	.write	= imx_iim_cdev_write,
	.lseek	= dev_lseek_default,
};

static int imx_iim_blow_enable_set(struct device_d *dev, struct param_d *param,
		const char *val)
{
	unsigned long blow_enable;

	if (val == NULL)
		return -EINVAL;

	blow_enable = simple_strtoul(val, NULL, 0);
	if (blow_enable > 1)
		return -EINVAL;

	return dev_param_set_generic(dev, param, blow_enable ? "1" : "0");
}

static int imx_iim_add_bank(struct device_d *dev, void __iomem *base, int num)
{
	struct iim_priv *priv;
	struct cdev *cdev;

	priv = xzalloc(sizeof (*priv));

	priv->base	= base;
	priv->bankbase	= priv->base + 0x800 + 0x400 * num;
	priv->bank	= num;
	priv->banksize	= 32;
	cdev = &priv->cdev;
	cdev->dev	= dev;
	cdev->ops	= &imx_iim_ops;
	cdev->priv	= priv;
	cdev->size	= 32;
	cdev->name	= asprintf(DRIVERNAME "_bank%d", num);
	if (cdev->name == NULL)
		return -ENOMEM;

	return devfs_create(cdev);
}

static int imx_iim_probe(struct device_d *dev)
{
	struct imx_iim_platform_data *pdata = dev->platform_data;
	int err;
	int i;
	void __iomem *base;

	base = dev_request_mem_region(dev, 0);

	if (pdata)
		mac_addr_base = pdata->mac_addr_base;

	for (i = 0; i < 8; i++) {
		imx_iim_add_bank(dev, base, i);
	}

#ifdef CONFIG_IMX_IIM_FUSE_BLOW
	err = dev_add_param(dev, "permanent_write_enable",
			imx_iim_blow_enable_set, NULL, 0);
	if (err < 0)
		return err;
	err = dev_set_param(dev, "permanent_write_enable", "0");
	if (err < 0)
		return err;
#endif /* CONFIG_IMX_IIM_FUSE_BLOW */

	err = dev_add_param(dev, "explicit_sense_enable",
			imx_iim_blow_enable_set, NULL, 0);
	if (err < 0)
		return err;
	err = dev_set_param(dev, "explicit_sense_enable", "1");
	if (err < 0)
		return err;

	return 0;
}

static struct driver_d imx_iim_driver = {
	.name	= DRIVERNAME,
	.probe	= imx_iim_probe,
};

static int imx_iim_init(void)
{
	register_driver(&imx_iim_driver);

	return 0;
}
coredevice_initcall(imx_iim_init);

int imx_iim_read(unsigned int bank, int offset, void *buf, int count)
{
	struct cdev *cdev;
	char *name = asprintf(DRIVERNAME "_bank%d", bank);
	int ret;

	cdev = cdev_open(name, O_RDONLY);
	if (!cdev)
		return -ENODEV;

	ret = cdev_read(cdev, buf, count, offset, 0);

	cdev_close(cdev);
	free(name);

	return ret;
}
