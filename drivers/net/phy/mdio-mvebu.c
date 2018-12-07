// SPDX-License-Identifier: GPL-2.0-only
/*
 * Marvell MVEBU SoC MDIO interface driver
 *
 * (C) Copyright 2014
 *   Pengutronix, Michael Grzeschik <mgr@pengutronix.de>
 *   Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 *
 * based on mvmdio driver from Linux
 *   Copyright (C) 2012 Marvell
 *     Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 *
 * Since the MDIO interface of Marvell network interfaces is shared
 * between all network interfaces, having a single driver allows to
 * handle concurrent accesses properly (you may have four Ethernet
 * ports, but they in fact share the same SMI interface to access
 * the MDIO bus). This driver is currently used by the mvneta and
 * mv643xx_eth drivers.
 */

#include <clock.h>
#include <common.h>
#include <driver.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/phy.h>

#define SMI_DATA_SHIFT          0
#define SMI_PHY_ADDR_SHIFT      16
#define SMI_PHY_REG_SHIFT       21
#define SMI_READ_OPERATION      BIT(26)
#define SMI_WRITE_OPERATION     0
#define SMI_READ_VALID          BIT(27)
#define SMI_BUSY                BIT(28)
#define ERR_INT_CAUSE		0x007C
#define  ERR_INT_SMI_DONE	BIT(4)
#define ERR_INT_MASK		BIT(7)

struct mdio_priv {
	struct mii_bus miibus;
	void __iomem *regs;
	struct clk *clk;
};

#define SMI_POLL_TIMEOUT	(10 * MSECOND)

static int mvebu_mdio_wait_ready(struct mdio_priv *priv)
{
	int ret = wait_on_timeout(SMI_POLL_TIMEOUT,
				  (readl(priv->regs) & SMI_BUSY) == 0);

	if (ret)
		dev_err(&priv->miibus.dev, "timeout, SMI busy for too long\n");

	return ret;
}

static int mvebu_mdio_read(struct mii_bus *bus, int addr, int reg)
{
	struct mdio_priv *priv = bus->priv;
	u32 smi;
	int ret;

	ret = mvebu_mdio_wait_ready(priv);
	if (ret)
		return ret;

	smi = SMI_READ_OPERATION;
	smi |= (addr << SMI_PHY_ADDR_SHIFT) | (reg << SMI_PHY_REG_SHIFT);
	writel(smi, priv->regs);

	ret = mvebu_mdio_wait_ready(priv);
	if (ret)
		return ret;

	smi = readl(priv->regs);
	if ((smi & SMI_READ_VALID) == 0) {
		dev_err(&bus->dev, "SMI bus read not valid\n");
		return -ENODEV;
	}

	return smi & 0xFFFF;
}

static int mvebu_mdio_write(struct mii_bus *bus, int addr, int reg, u16 data)
{
	struct mdio_priv *priv = bus->priv;
	u32 smi;
	int ret;

	ret = mvebu_mdio_wait_ready(priv);
	if (ret)
		return ret;

	smi = SMI_WRITE_OPERATION;
	smi |= (addr << SMI_PHY_ADDR_SHIFT) | (reg << SMI_PHY_REG_SHIFT);
	smi |= data << SMI_DATA_SHIFT;
	writel(smi, priv->regs);

	return 0;
}

static int mvebu_mdio_probe(struct device_d *dev)
{
	struct mdio_priv *priv;

	priv = xzalloc(sizeof(*priv));
	dev->priv = priv;

	priv->regs = dev_get_mem_region(dev, 0);
	if (IS_ERR(priv->regs))
		return PTR_ERR(priv->regs);

	priv->clk = clk_get(dev, NULL);
	if (IS_ERR(priv->clk))
		return PTR_ERR(priv->clk);
	clk_enable(priv->clk);

	priv->miibus.dev.device_node = dev->device_node;
	priv->miibus.priv = priv;
	priv->miibus.parent = dev;
	priv->miibus.read = mvebu_mdio_read;
	priv->miibus.write = mvebu_mdio_write;

	return mdiobus_register(&priv->miibus);
}

static void mvebu_mdio_remove(struct device_d *dev)
{
	struct mdio_priv *priv = dev->priv;

	mdiobus_unregister(&priv->miibus);

	clk_disable(priv->clk);
}

static struct of_device_id mvebu_mdio_dt_ids[] = {
	{ .compatible = "marvell,orion-mdio" },
	{ }
};

static struct driver_d mvebu_mdio_driver = {
	.name   = "mvebu-mdio",
	.probe  = mvebu_mdio_probe,
	.remove = mvebu_mdio_remove,
	.of_compatible = DRV_OF_COMPAT(mvebu_mdio_dt_ids),
};
device_platform_driver(mvebu_mdio_driver);
