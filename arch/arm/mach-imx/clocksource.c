/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Alex Zuepke <azu@sysgo.de>
 *
 * (C) Copyright 2002
 * Gary Jennejohn, DENX Software Engineering, <gj@denx.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 */

#include <common.h>
#include <init.h>
#include <clock.h>
#include <errno.h>
#include <of.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <notifier.h>
#include <io.h>

/* Part 1: Registers */
# define GPT_TCTL   0x00
# define GPT_TPRER  0x04

/* Part 2: Bitfields */
#define TCTL_SWR			(1 << 15)	/* Software reset */
#define IMX1_TCTL_FRR			(1 << 8)	/* Freerun / restart */
#define IMX31_TCTL_FRR			(1 << 9)	/* Freerun / restart */
#define IMX1_TCTL_CLKSOURCE_PER		(1 << 1)	/* Clock source bit position */
#define IMX31_TCTL_CLKSOURCE_IPG	(1 << 6)	/* Clock source bit position */
#define IMX31_TCTL_CLKSOURCE_PER	(2 << 6)	/* Clock source bit position */
#define TCTL_TEN			(1 << 0)	/* Timer enable */

static struct clk *clk_gpt;

struct imx_gpt_regs {
	unsigned int tcn;
	uint32_t tctl_val;
};

static struct imx_gpt_regs regs_imx1 = {
	.tcn = 0x10,
	.tctl_val = IMX1_TCTL_FRR | IMX1_TCTL_CLKSOURCE_PER | TCTL_TEN,
};

static struct imx_gpt_regs regs_imx31 = {
	.tcn = 0x24,
	.tctl_val = IMX31_TCTL_FRR | IMX31_TCTL_CLKSOURCE_PER | TCTL_TEN,
};

static struct imx_gpt_regs *regs;
static void __iomem *timer_base;

static uint64_t imx_clocksource_read(void)
{
	return readl(timer_base + regs->tcn);
}

static struct clocksource cs = {
	.read	= imx_clocksource_read,
	.mask	= CLOCKSOURCE_MASK(32),
	.shift	= 10,
};

static int imx_clocksource_clock_change(struct notifier_block *nb, unsigned long event, void *data)
{
	cs.mult = clocksource_hz2mult(clk_get_rate(clk_gpt), cs.shift);
	return 0;
}

static struct notifier_block imx_clock_notifier = {
	.notifier_call = imx_clocksource_clock_change,
};

static int imx_gpt_probe(struct device_d *dev)
{
	struct resource *iores;
	int i;
	int ret;
	unsigned long rate;

	/* one timer is enough */
	if (timer_base)
		return 0;

	ret = dev_get_drvdata(dev, (const void **)&regs);
	if (ret)
		return ret;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	timer_base = IOMEM(iores->start);

	/* setup GP Timer 1 */
	writel(TCTL_SWR, timer_base + GPT_TCTL);

	for (i = 0; i < 100; i++)
		writel(0, timer_base + GPT_TCTL); /* We have no udelay by now */

	clk_gpt = clk_get(dev, "per");
	if (IS_ERR(clk_gpt)) {
		rate = 20000000;
		dev_err(dev, "failed to get clock, assume %lu Hz\n", rate);
	} else {
		rate = clk_get_rate(clk_gpt);
		if (!rate) {
			dev_err(dev, "clock reports rate == 0\n");
			return -EIO;
		}
	}

	writel(0, timer_base + GPT_TPRER);
	writel(regs->tctl_val, timer_base + GPT_TCTL);

	cs.mult = clocksource_hz2mult(rate, cs.shift);

	init_clock(&cs);

	clock_register_client(&imx_clock_notifier);

	return 0;
}

static __maybe_unused struct of_device_id imx_gpt_dt_ids[] = {
	{
		.compatible = "fsl,imx1-gpt",
		.data = &regs_imx1,
	}, {
		.compatible = "fsl,imx21-gpt",
		.data = &regs_imx1,
	}, {
		.compatible = "fsl,imx31-gpt",
		.data = &regs_imx31,
	}, {
		.compatible = "fsl,imx6q-gpt",
		.data = &regs_imx31,
	}, {
		.compatible = "fsl,imx6dl-gpt",
		.data = &regs_imx31,
	}, {
		.compatible = "fsl,imx6ul-gpt",
		.data = &regs_imx31,
	}, {
		.compatible = "fsl,imx7d-gpt",
		.data = &regs_imx31,
	}, {
		/* sentinel */
	}
};

static struct platform_device_id imx_gpt_ids[] = {
	{
		.name = "imx1-gpt",
		.driver_data = (unsigned long)&regs_imx1,
	}, {
		.name = "imx31-gpt",
		.driver_data = (unsigned long)&regs_imx31,
	}, {
		/* sentinel */
	},
};

static struct driver_d imx_gpt_driver = {
	.name = "imx-gpt",
	.probe = imx_gpt_probe,
	.of_compatible = DRV_OF_COMPAT(imx_gpt_dt_ids),
	.id_table = imx_gpt_ids,
};

static int imx_gpt_init(void)
{
	return platform_driver_register(&imx_gpt_driver);
}
postcore_initcall(imx_gpt_init);
