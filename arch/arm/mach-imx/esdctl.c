/*
 * esdctl.c - i.MX sdram controller functions
 *
 * Copyright (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <io.h>
#include <sizes.h>
#include <init.h>
#include <asm/barebox-arm.h>
#include <asm/memory.h>
#include <mach/esdctl.h>
#include <mach/esdctl-v4.h>
#include <mach/imx1-regs.h>
#include <mach/imx21-regs.h>
#include <mach/imx25-regs.h>
#include <mach/imx27-regs.h>
#include <mach/imx31-regs.h>
#include <mach/imx35-regs.h>
#include <mach/imx51-regs.h>
#include <mach/imx53-regs.h>

struct imx_esdctl_data {
	unsigned long base0;
	unsigned long base1;
	void (*add_mem)(void *esdctlbase, struct imx_esdctl_data *);
};

/*
 * v1 - found on i.MX1
 */
static inline unsigned long imx_v1_sdram_size(void __iomem *esdctlbase, int num)
{
	void __iomem *esdctl = esdctlbase + (num ? 4 : 0);
	u32 ctlval = readl(esdctl);
	unsigned long size;
	int rows, cols, width = 2, banks = 4;

	if (!(ctlval & ESDCTL0_SDE))
		/* SDRAM controller disabled, so no RAM here */
		return 0;

	rows = ((ctlval >> 24) & 0x3) + 11;
	cols = ((ctlval >> 20) & 0x3) + 8;

	if (ctlval & (1 << 17))
		width = 4;

	size = (1 << cols) * (1 << rows) * banks * width;

	if (size > SZ_64M)
		size = SZ_64M;

	return size;
}

/*
 * v2 - found on i.MX25, i.MX27, i.MX31 and i.MX35
 */
static inline unsigned long imx_v2_sdram_size(void __iomem *esdctlbase, int num)
{
	void __iomem *esdctl = esdctlbase + (num ? IMX_ESDCTL1 : IMX_ESDCTL0);
	u32 ctlval = readl(esdctl);
	unsigned long size;
	int rows, cols, width = 2, banks = 4;

	if (!(ctlval & ESDCTL0_SDE))
		/* SDRAM controller disabled, so no RAM here */
		return 0;

	rows = ((ctlval >> 24) & 0x7) + 11;
	cols = ((ctlval >> 20) & 0x3) + 8;

	if ((ctlval & ESDCTL0_DSIZ_MASK) == ESDCTL0_DSIZ_31_0)
		width = 4;

	size = (1 << cols) * (1 << rows) * banks * width;

	if (size > SZ_256M)
		size = SZ_256M;

	return size;
}

/*
 * v3 - found on i.MX51
 */
static inline unsigned long imx_v3_sdram_size(void __iomem *esdctlbase, int num)
{
	unsigned long size;

	size = imx_v2_sdram_size(esdctlbase, num);

	if (readl(esdctlbase + IMX_ESDMISC) & (1 << 6))
		size *= 2;

	if (size > SZ_256M)
		size = SZ_256M;

	return size;
}

/*
 * v4 - found on i.MX53
 */
static inline unsigned long imx_v4_sdram_size(void __iomem *esdctlbase, int cs)
{
	u32 ctlval = readl(esdctlbase + ESDCTL_V4_ESDCTL0);
	u32 esdmisc = readl(esdctlbase + ESDCTL_V4_ESDMISC);
	unsigned long size;
	int rows, cols, width = 2, banks = 8;

	if (cs == 0 && !(ctlval & ESDCTL_V4_ESDCTLx_SDE0))
		return 0;
	if (cs == 1 && !(ctlval & ESDCTL_V4_ESDCTLx_SDE1))
		return 0;

	/* one 2GiB cs, memory is returned for cs0 only */
	if (cs == 1 && (esdmisc & ESDCTL_V4_ESDMISC_ONE_CS))
		return 9;

	rows = ((ctlval >> 24) & 0x7) + 11;
	switch ((ctlval >> 20) & 0x7) {
	case 0:
		cols = 9;
		break;
	case 1:
		cols = 10;
		break;
	case 2:
		cols = 11;
		break;
	case 3:
		cols = 8;
		break;
	case 4:
		cols = 12;
		break;
	default:
		cols = 0;
		break;
	}

	if (ctlval & ESDCTL_V4_ESDCTLx_DSIZ_32B)
		width = 4;

	if (esdmisc & ESDCTL_V4_ESDMISC_BANKS_4)
		banks = 4;

	size = (1 << cols) * (1 << rows) * banks * width;

	return size;
}

static void add_mem(unsigned long base0, unsigned long size0,
		unsigned long base1, unsigned long size1)
{
	debug("%s: cs0 base: 0x%08x cs0 size: 0x%08x\n", __func__, base0, size0);
	debug("%s: cs1 base: 0x%08x cs1 size: 0x%08x\n", __func__, base1, size1);

	if (base0 + size0 == base1 && size1 > 0) {
		/*
		 * concatenate both chip selects to a single bank
		 */
		arm_add_mem_device("ram0", base0, size0 + size1);

		return;
	}

	if (size0)
		arm_add_mem_device("ram0", base0, size0);

	if (size1)
		arm_add_mem_device(size0 ? "ram1" : "ram0", base1, size1);
}

static void imx_esdctl_v1_add_mem(void *esdctlbase, struct imx_esdctl_data *data)
{
	add_mem(data->base0, imx_v1_sdram_size(esdctlbase, 0),
			data->base1, imx_v1_sdram_size(esdctlbase, 1));
}

static void imx_esdctl_v2_add_mem(void *esdctlbase, struct imx_esdctl_data *data)
{
	add_mem(data->base0, imx_v2_sdram_size(esdctlbase, 0),
			data->base1, imx_v2_sdram_size(esdctlbase, 1));
}

static void imx_esdctl_v3_add_mem(void *esdctlbase, struct imx_esdctl_data *data)
{
	add_mem(data->base0, imx_v3_sdram_size(esdctlbase, 0),
			data->base1, imx_v3_sdram_size(esdctlbase, 1));
}

static void imx_esdctl_v4_add_mem(void *esdctlbase, struct imx_esdctl_data *data)
{
	add_mem(data->base0, imx_v4_sdram_size(esdctlbase, 0),
			data->base1, imx_v4_sdram_size(esdctlbase, 1));
}

static int imx_esdctl_probe(struct device_d *dev)
{
	struct imx_esdctl_data *data;
	int ret;
	void *base;

	ret = dev_get_drvdata(dev, (unsigned long *)&data);
	if (ret)
		return ret;

	base = dev_request_mem_region(dev, 0);
	if (!base)
		return -ENOMEM;

	data->add_mem(base, data);

	return 0;
}

static __maybe_unused struct imx_esdctl_data imx1_data = {
	.base0 = MX1_CSD0_BASE_ADDR,
	.base1 = MX1_CSD1_BASE_ADDR,
	.add_mem = imx_esdctl_v1_add_mem,
};

static __maybe_unused struct imx_esdctl_data imx25_data = {
	.base0 = MX25_CSD0_BASE_ADDR,
	.base1 = MX25_CSD1_BASE_ADDR,
	.add_mem = imx_esdctl_v2_add_mem,
};

static __maybe_unused struct imx_esdctl_data imx27_data = {
	.base0 = MX27_CSD0_BASE_ADDR,
	.base1 = MX27_CSD1_BASE_ADDR,
	.add_mem = imx_esdctl_v2_add_mem,
};

static __maybe_unused struct imx_esdctl_data imx31_data = {
	.base0 = MX31_CSD0_BASE_ADDR,
	.base1 = MX31_CSD1_BASE_ADDR,
	.add_mem = imx_esdctl_v2_add_mem,
};

static __maybe_unused struct imx_esdctl_data imx35_data = {
	.base0 = MX35_CSD0_BASE_ADDR,
	.base1 = MX35_CSD1_BASE_ADDR,
	.add_mem = imx_esdctl_v2_add_mem,
};

static __maybe_unused struct imx_esdctl_data imx51_data = {
	.base0 = MX51_CSD0_BASE_ADDR,
	.base1 = MX51_CSD1_BASE_ADDR,
	.add_mem = imx_esdctl_v3_add_mem,
};

static __maybe_unused struct imx_esdctl_data imx53_data = {
	.base0 = MX53_CSD0_BASE_ADDR,
	.base1 = MX53_CSD1_BASE_ADDR,
	.add_mem = imx_esdctl_v4_add_mem,
};

static struct platform_device_id imx_esdctl_ids[] = {
#ifdef CONFIG_ARCH_IMX1
	{
		.name = "imx1-sdramc",
		.driver_data = (unsigned long)&imx1_data,
	},
#endif
#ifdef CONFIG_ARCH_IMX25
	{
		.name = "imx25-esdctl",
		.driver_data = (unsigned long)&imx25_data,
	},
#endif
#ifdef CONFIG_ARCH_IMX27
	{
		.name = "imx27-esdctl",
		.driver_data = (unsigned long)&imx27_data,
	},
#endif
#ifdef CONFIG_ARCH_IMX31
	{
		.name = "imx31-esdctl",
		.driver_data = (unsigned long)&imx31_data,
	},
#endif
#ifdef CONFIG_ARCH_IMX35
	{
		.name = "imx35-esdctl",
		.driver_data = (unsigned long)&imx35_data,
	},
#endif
#ifdef CONFIG_ARCH_IMX51
	{
		.name = "imx51-esdctl",
		.driver_data = (unsigned long)&imx51_data,
	},
#endif
#ifdef CONFIG_ARCH_IMX53
	{
		.name = "imx53-esdctl",
		.driver_data = (unsigned long)&imx53_data,
	},
#endif
	{
		/* sentinel */
	},
};

static struct driver_d imx_serial_driver = {
	.name   = "imx-esdctl",
	.probe  = imx_esdctl_probe,
	.id_table = imx_esdctl_ids,
};

static int imx_esdctl_init(void)
{
	return platform_driver_register(&imx_serial_driver);
}

mem_initcall(imx_esdctl_init);
