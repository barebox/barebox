/*
 *  (C) 2007, Sascha Hauer <sha@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 */

#include <common.h>
#include <io.h>
#include <init.h>
#include <linux/err.h>
#include <mach/iomux-mx31.h>

/*
 * IOMUX register (base) addresses
 */
#define IOMUXINT_OBS1	0x000
#define IOMUXINT_OBS2	0x004
#define IOMUXGPR	0x008
#define IOMUXSW_MUX_CTL	0x00C
#define IOMUXSW_PAD_CTL	0x154

#define IOMUX_REG_MASK (IOMUX_PADNUM_MASK & ~0x3)

static void __iomem *base;

/*
 * set the mode for a IOMUX pin.
 */
int imx_iomux_mode(unsigned int pin_mode)
{
	u32 field, l, mode, ret = 0;
	void __iomem *reg;

	if (!base)
		return -EINVAL;

	reg = base + IOMUXSW_MUX_CTL + (pin_mode & IOMUX_REG_MASK);
	field = pin_mode & 0x3;
	mode = (pin_mode & IOMUX_MODE_MASK) >> IOMUX_MODE_SHIFT;

	pr_debug("%s: reg offset = 0x%x field = %d mode = 0x%02x\n",
			__func__, (pin_mode & IOMUX_REG_MASK), field, mode);

	l = readl(reg);
	l &= ~(0xff << (field * 8));
	l |= mode << (field * 8);
	writel(l, reg);

	return ret;
}
EXPORT_SYMBOL(imx_iomux_mode);

/*
 * This function configures the pad value for a IOMUX pin.
 */
void imx_iomux_set_pad(enum iomux_pins pin, u32 config)
{
	u32 field, l;
	void __iomem *reg;

	if (!base)
		return;

	pin &= IOMUX_PADNUM_MASK;
	reg = base + IOMUXSW_PAD_CTL + (pin + 2) / 3 * 4;
	field = (pin + 2) % 3;

	pr_debug("%s: reg offset = 0x%x, field = %d\n",
			__func__, (pin + 2) / 3, field);

	l = readl(reg);
	l &= ~(0x1ff << (field * 10));
	l |= config << (field * 10);
	writel(l, reg);
}
EXPORT_SYMBOL(imx_iomux_set_pad);

/*
 * This function enables/disables the general purpose function for a particular
 * signal.
 */
void imx_iomux_set_gpr(enum iomux_gp_func gp, bool en)
{
	u32 l;

	if (!base)
		return;

	l = readl(base + IOMUXGPR);
	if (en)
		l |= gp;
	else
		l &= ~gp;

	writel(l, base + IOMUXGPR);
}
EXPORT_SYMBOL(imx_iomux_set_gpr);

int imx_iomux_setup_multiple_pins(const unsigned int *pin_list, unsigned count)
{
	int i;

	for (i = 0; i < count; i++)
		imx_iomux_mode(pin_list[i]);

	return 0;
}

static int imx_iomux_probe(struct device_d *dev)
{
	struct resource *iores;
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	base = IOMEM(iores->start);

	return 0;
}

static __maybe_unused struct of_device_id imx_iomux_dt_ids[] = {
	{
		.compatible = "fsl,imx31-iomux",
	}, {
		/* sentinel */
	}
};

static struct platform_device_id imx_iomux_ids[] = {
	{
		.name = "imx31-iomux",
	}, {
		/* sentinel */
	},
};

static struct driver_d imx_iomux_driver = {
	.name = "imx-iomuxv2",
	.probe = imx_iomux_probe,
	.of_compatible = DRV_OF_COMPAT(imx_iomux_dt_ids),
	.id_table = imx_iomux_ids,
};

static int imx_iomux_init(void)
{
	return platform_driver_register(&imx_iomux_driver);
}
core_initcall(imx_iomux_init);
