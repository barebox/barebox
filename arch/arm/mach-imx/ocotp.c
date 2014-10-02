/*
 * ocotp.c - i.MX6 ocotp fusebox driver
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
#include <driver.h>
#include <malloc.h>
#include <xfuncs.h>
#include <errno.h>
#include <init.h>
#include <net.h>
#include <io.h>
#include <of.h>
#include <clock.h>
#include <linux/clk.h>

/*
 * a single MAC address reference has the form
 * <&phandle regoffset>
 */
#define MAC_ADDRESS_PROPLEN	(2 * sizeof(__be32))

/* OCOTP Registers offsets */
#define OCOTP_CTRL_SET			0x04
#define OCOTP_CTRL_CLR			0x08
#define OCOTP_TIMING			0x10
#define OCOTP_DATA			0x20
#define OCOTP_READ_CTRL			0x30
#define OCOTP_READ_FUSE_DATA		0x40

/* OCOTP Registers bits and masks */
#define OCOTP_CTRL_WR_UNLOCK		16
#define OCOTP_CTRL_WR_UNLOCK_KEY	0x3E77
#define OCOTP_CTRL_WR_UNLOCK_MASK	0xFFFF0000
#define OCOTP_CTRL_ADDR			0
#define OCOTP_CTRL_ADDR_MASK		0x0000007F
#define OCOTP_CTRL_BUSY			(1 << 8)
#define OCOTP_CTRL_ERROR		(1 << 9)
#define OCOTP_CTRL_RELOAD_SHADOWS	(1 << 10)

#define OCOTP_TIMING_STROBE_READ	16
#define OCOTP_TIMING_STROBE_READ_MASK	0x003F0000
#define OCOTP_TIMING_RELAX		12
#define OCOTP_TIMING_RELAX_MASK		0x0000F000
#define OCOTP_TIMING_STROBE_PROG	0
#define OCOTP_TIMING_STROBE_PROG_MASK	0x00000FFF

#define OCOTP_READ_CTRL_READ_FUSE	0x00000001

#define BF(value, field)		(((value) << field) & field##_MASK)

/* Other definitions */
#define FUSE_REGS_COUNT			(16 * 8)
#define IMX6_OTP_DATA_ERROR_VAL		0xBADABADA
#define DEF_RELAX			20
#define MAC_OFFSET			(0x22 * 4)
#define MAC_BYTES			8

struct ocotp_priv {
	struct cdev cdev;
	void __iomem *base;
	struct clk *clk;
	struct device_d dev;
	int permanent_write_enable;
	int sense_enable;
	char ethaddr[6];
};

static int imx6_ocotp_set_timing(struct ocotp_priv *priv)
{
	u32 clk_rate;
	u32 relax, strobe_read, strobe_prog;
	u32 timing;

	clk_rate = clk_get_rate(priv->clk);

	relax = clk_rate / (1000000000 / DEF_RELAX) - 1;
	strobe_prog = clk_rate / (1000000000 / 10000) + 2 * (DEF_RELAX + 1) - 1;
	strobe_read = clk_rate / (1000000000 / 40) + 2 * (DEF_RELAX + 1) - 1;

	timing = BF(relax, OCOTP_TIMING_RELAX);
	timing |= BF(strobe_read, OCOTP_TIMING_STROBE_READ);
	timing |= BF(strobe_prog, OCOTP_TIMING_STROBE_PROG);

	writel(timing, priv->base + OCOTP_TIMING);

	return 0;
}

static int imx6_ocotp_wait_busy(u32 flags, struct ocotp_priv *priv)
{
	uint64_t start = get_time_ns();

	while ((OCOTP_CTRL_BUSY | OCOTP_CTRL_ERROR | flags) &
			readl(priv->base)) {
		if (is_timeout(start, MSECOND)) {
			/* Clear ERROR bit */
			writel(OCOTP_CTRL_ERROR, priv->base + OCOTP_CTRL_CLR);
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static int imx6_ocotp_prepare(struct ocotp_priv *priv)
{
	int ret;

	ret = imx6_ocotp_set_timing(priv);
	if (ret)
		return ret;

	ret = imx6_ocotp_wait_busy(0, priv);
	if (ret)
		return ret;

	return 0;
}

static int fuse_read_addr(u32 addr, u32 *pdata, struct ocotp_priv *priv)
{
	u32 ctrl_reg;
	int ret;

	ctrl_reg = readl(priv->base);
	ctrl_reg &= ~OCOTP_CTRL_ADDR_MASK;
	ctrl_reg &= ~OCOTP_CTRL_WR_UNLOCK_MASK;
	ctrl_reg |= BF(addr, OCOTP_CTRL_ADDR);
	writel(ctrl_reg, priv->base);

	writel(OCOTP_READ_CTRL_READ_FUSE, priv->base + OCOTP_READ_CTRL);
	ret = imx6_ocotp_wait_busy(0, priv);
	if (ret)
		return ret;

	*pdata = readl(priv->base + OCOTP_READ_FUSE_DATA);

	return 0;
}

int imx6_ocotp_read_one_u32(u32 index, u32 *pdata, struct ocotp_priv *priv)
{
	int ret;

	ret = imx6_ocotp_prepare(priv);
	if (ret) {
		dev_err(priv->cdev.dev, "failed to prepare read fuse 0x%08x\n",
				index);
		return ret;
	}

	ret = fuse_read_addr(index, pdata, priv);
	if (ret) {
		dev_err(priv->cdev.dev, "failed to read fuse 0x%08x\n", index);
		return ret;
	}

	if (readl(priv->base) & OCOTP_CTRL_ERROR) {
		dev_err(priv->cdev.dev, "bad read status at fuse 0x%08x\n", index);
		return -EFAULT;
	}

	return 0;
}

static ssize_t imx6_ocotp_cdev_read(struct cdev *cdev, void *buf,
		size_t count, loff_t offset, unsigned long flags)
{
	u32 index;
	ssize_t read_count = 0;
	int ret, i;
	struct ocotp_priv *priv = container_of(cdev, struct ocotp_priv, cdev);

	index = offset >> 2;
	count >>= 2;

	if (count > (FUSE_REGS_COUNT - index))
		count = FUSE_REGS_COUNT - index - 1;

	for (i = index; i < (index + count); i++) {
		if (priv->sense_enable) {
			ret = imx6_ocotp_read_one_u32(i, buf, priv);
			if (ret)
				return ret;
		} else {
			*(u32 *)buf = readl(priv->base + 0x400 + i * 0x10);
		}

		buf += 4;
		read_count++;
	}

	return read_count << 2;
}

static int fuse_blow_addr(u32 addr, u32 value, struct ocotp_priv *priv)
{
	u32 ctrl_reg;
	int ret;

	/* Control register */
	ctrl_reg = readl(priv->base);
	ctrl_reg &= ~OCOTP_CTRL_ADDR_MASK;
	ctrl_reg |= BF(addr, OCOTP_CTRL_ADDR);
	ctrl_reg |= BF(OCOTP_CTRL_WR_UNLOCK_KEY, OCOTP_CTRL_WR_UNLOCK);
	writel(ctrl_reg, priv->base);

	writel(value, priv->base + OCOTP_DATA);
	ret = imx6_ocotp_wait_busy(0, priv);
	if (ret)
		return ret;

	/* Write postamble */
	udelay(2000);
	return 0;
}

static int imx6_ocotp_reload_shadow(struct ocotp_priv *priv)
{
	dev_info(priv->cdev.dev, "reloading shadow registers...\n");
	writel(OCOTP_CTRL_RELOAD_SHADOWS, priv->base + OCOTP_CTRL_SET);
	udelay(1);

	return imx6_ocotp_wait_busy(OCOTP_CTRL_RELOAD_SHADOWS, priv);
}

int imx6_ocotp_blow_one_u32(u32 index, u32 data, u32 *pfused_value,
		struct ocotp_priv *priv)
{
	int ret;

	ret = imx6_ocotp_prepare(priv);
	if (ret) {
		dev_err(priv->cdev.dev, "prepare to write failed\n");
		return ret;
	}

	ret = fuse_blow_addr(index, data, priv);
	if (ret) {
		dev_err(priv->cdev.dev, "fuse blow failed\n");
		return ret;
	}

	if (readl(priv->base) & OCOTP_CTRL_ERROR) {
		dev_err(priv->cdev.dev, "bad write status\n");
		return -EFAULT;
	}

	ret = imx6_ocotp_read_one_u32(index, pfused_value, priv);

	return ret;
}

static ssize_t imx6_ocotp_cdev_write(struct cdev *cdev, const void *buf,
		size_t count, loff_t offset, unsigned long flags)
{
	struct ocotp_priv *priv = cdev->priv;
	int index, i;
	ssize_t write_count = 0;
	const u32 *data;
	u32 pfuse;
	int ret;

	/* We could do better, but currently this is what's implemented */
	if (offset & 0x3 || count & 0x3) {
		dev_err(cdev->dev, "only u32 aligned writes allowed\n");
		return -EINVAL;
	}

	index = offset >> 2;
	count >>= 2;

	if (count > (FUSE_REGS_COUNT - index))
		count = FUSE_REGS_COUNT - index - 1;

	data = buf;

	for (i = index; i < (index + count); i++) {
		if (priv->permanent_write_enable) {
			ret = imx6_ocotp_blow_one_u32(i, *data,
					&pfuse, priv);
			if (ret < 0) {
				goto out;
			}
		} else {
			writel(*data, priv->base + 0x400 + i * 0x10);
		}

		data++;
		write_count++;
	}

	ret = 0;

out:
	if (priv->permanent_write_enable)
		imx6_ocotp_reload_shadow(priv);

	return ret < 0 ? ret : (write_count << 2);
}

static struct file_operations imx6_ocotp_ops = {
	.read	= imx6_ocotp_cdev_read,
	.write	= imx6_ocotp_cdev_write,
	.lseek	= dev_lseek_default,
};

static void imx_ocotp_init_dt(struct device_d *dev, void __iomem *base)
{
	char mac[6];
	const __be32 *prop;
	struct device_node *node = dev->device_node;
	int len;

	if (!node)
		return;

	prop = of_get_property(node, "barebox,provide-mac-address", &len);
	if (!prop)
		return;

	while (len >= MAC_ADDRESS_PROPLEN) {
		struct device_node *rnode;
		uint32_t phandle, offset, value;

		phandle = be32_to_cpup(prop++);

		rnode = of_find_node_by_phandle(phandle);
		offset = be32_to_cpup(prop++);

		value = readl(base + offset + 0x10);
		mac[0] = (value >> 8);
		mac[1] = value;
		value = readl(base + offset);
		mac[2] = value >> 24;
		mac[3] = value >> 16;
		mac[4] = value >> 8;
		mac[5] = value;

		of_eth_register_ethaddr(rnode, mac);

		len -= MAC_ADDRESS_PROPLEN;
	}
}

static int imx_ocotp_get_mac(struct param_d *param, void *priv)
{
	struct ocotp_priv *ocotp_priv = priv;
	char buf[8];
	int i;

	imx6_ocotp_cdev_read(&ocotp_priv->cdev, buf, MAC_BYTES, MAC_OFFSET, 0);

	for (i = 0; i < 6; i++)
		ocotp_priv->ethaddr[i] = buf[5 - i];

	return 0;
}

static int imx_ocotp_set_mac(struct param_d *param, void *priv)
{
	struct ocotp_priv *ocotp_priv = priv;
	char buf[8];
	int i, ret;

	for (i = 0; i < 6; i++)
		buf[5 - i] = ocotp_priv->ethaddr[i];
	buf[6] = 0; buf[7] = 0;

	ret = imx6_ocotp_cdev_write(&ocotp_priv->cdev, buf, MAC_BYTES, MAC_OFFSET, 0);
	if (ret < 0)
		return ret;

	return 0;
}

static int imx_ocotp_probe(struct device_d *dev)
{
	void __iomem *base;
	struct ocotp_priv *priv;
	struct cdev *cdev;
	int ret = 0;

	base = dev_request_mem_region(dev, 0);
	if (IS_ERR(base))
		return PTR_ERR(base);

	imx_ocotp_init_dt(dev, base);

	priv = xzalloc(sizeof(*priv));

	priv->base	= base;
	priv->clk	= clk_get(dev, NULL);
	if (IS_ERR(priv->clk))
		return PTR_ERR(priv->clk);

	cdev		= &priv->cdev;
	cdev->dev	= dev;
	cdev->ops	= &imx6_ocotp_ops;
	cdev->priv	= priv;
	cdev->size	= 192;
	cdev->name	= "imx-ocotp";
	if (cdev->name == NULL)
		return -ENOMEM;

	ret = devfs_create(cdev);

	if (ret < 0)
		return ret;

	strcpy(priv->dev.name, "ocotp");
	priv->dev.parent = dev;
	register_device(&priv->dev);

	if (IS_ENABLED(CONFIG_IMX_OCOTP_WRITE)) {
		dev_add_param_bool(&(priv->dev), "permanent_write_enable",
				NULL, NULL, &priv->permanent_write_enable, NULL);
	}

	dev_add_param_mac(&(priv->dev), "mac_addr", imx_ocotp_set_mac,
			imx_ocotp_get_mac, priv->ethaddr, priv);
	dev_add_param_bool(&(priv->dev), "sense_enable", NULL, NULL, &priv->sense_enable, priv);

	return 0;
}

static __maybe_unused struct of_device_id imx_ocotp_dt_ids[] = {
	{
		.compatible = "fsl,imx6q-ocotp",
	}, {
		/* sentinel */
	}
};

static struct driver_d imx_ocotp_driver = {
	.name	= "imx_ocotp",
	.probe	= imx_ocotp_probe,
	.of_compatible = DRV_OF_COMPAT(imx_ocotp_dt_ids),
};

static int imx_ocotp_init(void)
{
	platform_driver_register(&imx_ocotp_driver);

	return 0;
}
coredevice_initcall(imx_ocotp_init);
