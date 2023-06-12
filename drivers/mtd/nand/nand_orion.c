// SPDX-License-Identifier: GPL-2.0-only
/*
 * (C) Copyright 2014, Ezequiel Garcia <ezequiel.garcia@free-electrons.com>
 *
 * Based on Orion NAND driver from Linux (drivers/mtd/nand/orion_nand.c):
 * Author: Tzachi Perelstein <tzachi@marvell.com>
 */

#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <init.h>
#include <io.h>
#include <of_mtd.h>
#include <errno.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/rawnand.h>
#include <linux/clk.h>

struct orion_nand {
	struct nand_chip chip;

	u8 ale;         /* address line number connected to ALE */
	u8 cle;         /* address line number connected to CLE */
};

static void orion_nand_cmd_ctrl(struct nand_chip *chip, int cmd, unsigned int ctrl)
{
	struct orion_nand *priv = chip->priv;
	u32 offs;

	if (cmd == NAND_CMD_NONE)
		return;

	if (ctrl & NAND_CLE)
		offs = (1 << priv->cle);
	else if (ctrl & NAND_ALE)
		offs = (1 << priv->ale);
	else
		return;

	if (chip->options & NAND_BUSWIDTH_16)
		offs <<= 1;

	writeb(cmd, chip->legacy.IO_ADDR_W + offs);
}

static void orion_nand_read_buf(struct nand_chip *chip, uint8_t *buf, int len)
{
	void __iomem *io_base = chip->legacy.IO_ADDR_R;
	uint64_t *buf64;
	int i = 0;

	while (len && (unsigned long)buf & 7) {
		*buf++ = readb(io_base);
		len--;
	}
	buf64 = (uint64_t *)buf;
	while (i < len/8) {
		/*
		 * Since GCC has no proper constraint (PR 43518)
		 * force x variable to r2/r3 registers as ldrd instruction
		 * requires first register to be even.
		 */
		register uint64_t x asm ("r2");

		asm volatile ("ldrd\t%0, [%1]" : "=&r" (x) : "r" (io_base));
		buf64[i++] = x;
	}
	i *= 8;
	while (i < len)
		buf[i++] = readb(io_base);
}

static int orion_nand_probe(struct device *dev)
{
	struct resource *iores;
	struct device_node *dev_node = dev->of_node;
	struct orion_nand *priv;
	struct mtd_info *mtd;
	struct nand_chip *chip;
	struct clk *clk;
	void __iomem *io_base;
	int width, ret;
	u32 val = 0;

	priv = xzalloc(sizeof(struct orion_nand));
	chip = &priv->chip;
	mtd = nand_to_mtd(chip);

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	io_base = IOMEM(iores->start);

	if (!of_property_read_u32(dev_node, "cle", &val))
		priv->cle = (u8)val;
	else
		priv->cle = 0;

	if (!of_property_read_u32(dev_node, "ale", &val))
		priv->ale = (u8)val;
	else
		priv->ale = 1;

	if (!of_property_read_u32(dev_node, "bank-width", &val))
		width = (u8)val * 8;
	else
		width = 8;

	if (!of_property_read_u32(dev_node, "chip-delay", &val))
		chip->legacy.chip_delay = (u8)val;

	mtd->dev.parent = dev;
	chip->priv = priv;
	chip->legacy.IO_ADDR_R = chip->legacy.IO_ADDR_W = io_base;
	chip->legacy.cmd_ctrl = orion_nand_cmd_ctrl;
	chip->legacy.read_buf = orion_nand_read_buf;
	chip->ecc.mode = NAND_ECC_SOFT;

	WARN(width > 16, "%d bit bus width out of range", width);
	if (width == 16)
		chip->options |= NAND_BUSWIDTH_16;

	/* Not all platforms can gate the clock, so this is optional */
	clk = clk_get(dev, 0);
	if (!IS_ERR(clk))
		clk_enable(clk);

	if (nand_scan(chip, 1)) {
		ret = -ENXIO;
		goto no_dev;
	}

	add_mtd_nand_device(mtd, "orion_nand");
	return 0;
no_dev:
	if (!IS_ERR(clk))
		clk_disable(clk);

	free(priv);
	return ret;
}

static __maybe_unused struct of_device_id orion_nand_compatible[] = {
	{ .compatible = "marvell,orion-nand", },
	{},
};
MODULE_DEVICE_TABLE(of, orion_nand_compatible);

static struct driver orion_nand_driver = {
	.name  = "orion_nand",
	.probe = orion_nand_probe,
	.of_compatible = DRV_OF_COMPAT(orion_nand_compatible),
};
device_platform_driver(orion_nand_driver);
