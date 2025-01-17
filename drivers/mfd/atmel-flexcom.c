// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: (C) 2015 Atmel Corporation
/*
 * Driver for Atmel Flexcom
 * Author: Cyrille Pitchen <cyrille.pitchen@atmel.com>
 */

#include <common.h>
#include <of.h>
#include <linux/clk.h>
#include <dt-bindings/mfd/atmel-flexcom.h>

/* I/O register offsets */
#define FLEX_MR		0x0	/* Mode Register */
#define FLEX_VERSION	0xfc	/* Version Register */

/* Mode Register bit fields */
#define FLEX_MR_OPMODE_OFFSET	(0)  /* Operating Mode */
#define FLEX_MR_OPMODE_MASK	(0x3 << FLEX_MR_OPMODE_OFFSET)
#define FLEX_MR_OPMODE(opmode)	(((opmode) << FLEX_MR_OPMODE_OFFSET) &	\
				 FLEX_MR_OPMODE_MASK)

static int atmel_flexcom_probe(struct device *dev)
{
	void __iomem *base;
	struct clk *clk;
	u32 opmode;
	int err;

	err = of_property_read_u32(dev->of_node,
				   "atmel,flexcom-mode", &opmode);
	if (err)
		return err;

	if (opmode < ATMEL_FLEXCOM_MODE_USART || opmode > ATMEL_FLEXCOM_MODE_TWI)
		return -EINVAL;

	base = dev_platform_get_and_ioremap_resource(dev, 0, NULL);
	if (IS_ERR(base))
		return dev_err_probe(dev, PTR_ERR(base), "Could not get memory region\n");

	clk = clk_get_enabled(dev, NULL);
	if (IS_ERR(clk))
		return dev_err_probe(dev, PTR_ERR(clk), "Can't get clk\n");

	/*
	 * Set the Operating Mode in the Mode Register: only the selected device
	 * is clocked. Hence, registers of the other serial devices remain
	 * inaccessible and are read as zero. Also the external I/O lines of the
	 * Flexcom are muxed to reach the selected device.
	 */
	writel(FLEX_MR_OPMODE(opmode), base + FLEX_MR);

	clk_disable(clk);

	return of_platform_populate(dev->of_node, NULL, dev);
}

static const struct of_device_id atmel_flexcom_of_match[] = {
	{ .compatible = "atmel,sama5d2-flexcom" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, atmel_flexcom_of_match);

static struct driver atmel_flexcom_driver = {
	.probe		= atmel_flexcom_probe,
	.name		= "atmel_flexcom",
	.of_compatible	= atmel_flexcom_of_match,
};
coredevice_platform_driver(atmel_flexcom_driver);
