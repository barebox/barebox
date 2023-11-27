// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <errno.h>
#include <malloc.h>
#include <restart.h>
#include <watchdog.h>
#include <reset_source.h>
#include <linux/clk.h>
#include <asm/system.h>

struct imxulp_socdata {
	unsigned int rate;
};

struct imxulp_wd {
	struct watchdog wd;
	void __iomem *base;
	unsigned int rate;
	struct device *dev;
};

#define REFRESH_WORD0 0xA602 /* 1st refresh word */
#define REFRESH_WORD1 0xB480 /* 2nd refresh word */

#define UNLOCK_WORD0 0xC520 /* 1st unlock word */
#define UNLOCK_WORD1 0xD928 /* 2nd unlock word */

#define UNLOCK_WORD 0xD928C520 /* unlock word */
#define REFRESH_WORD 0xB480A602 /* refresh word */

#define WDOG_CS                 0x0
#define WDOG_CS_UPDATE		BIT(5)
#define WDOG_CS_EN		BIT(7)
#define WDOG_CS_RCS		BIT(10)
#define WDOG_CS_ULK		BIT(11)
#define WDOG_CS_PRES		BIT(12)
#define WDOG_CS_CMD32EN		BIT(13)
#define WDOG_CS_FLG		BIT(14)
#define WDOG_CS_INT		BIT(6)
#define WDOG_CS_LPO_CLK		(0x1 << 8)

#define WDOG_CNT		0x4
#define WDOG_TOVAL		0x8

#define CLK_RATE_1KHZ		1000
#define CLK_RATE_32KHZ		125

static int imxulp_watchdog_set_timeout(struct watchdog *wd, unsigned int timeout)
{
	struct imxulp_wd *imxwd = container_of(wd, struct imxulp_wd, wd);
	u32 cmd32 = 0;

	if (timeout == 0)
		return -ENOSYS;

	if (readl(imxwd->base + WDOG_CS) & WDOG_CS_CMD32EN) {
		writel(UNLOCK_WORD, imxwd->base + WDOG_CNT);
		cmd32 = WDOG_CS_CMD32EN;
	} else {
		writel(UNLOCK_WORD0, imxwd->base + WDOG_CNT);
		writel(UNLOCK_WORD1, imxwd->base + WDOG_CNT);
	}

	/* Wait WDOG Unlock */
	while (!(readl(imxwd->base + WDOG_CS) & WDOG_CS_ULK))
		;

	writel(timeout * imxwd->rate, imxwd->base + WDOG_TOVAL);

	writel(cmd32 | WDOG_CS_EN | WDOG_CS_UPDATE | WDOG_CS_LPO_CLK |
	       WDOG_CS_FLG | WDOG_CS_PRES | WDOG_CS_INT, imxwd->base + WDOG_CS);

	/* Wait WDOG reconfiguration */
	while (!(readl(imxwd->base + WDOG_CS) & WDOG_CS_RCS))
		;

	if (readl(imxwd->base + WDOG_CS) & WDOG_CS_CMD32EN) {
		writel(REFRESH_WORD, imxwd->base + WDOG_CNT);
	} else {
		writel(REFRESH_WORD0, imxwd->base + WDOG_CNT);
		writel(REFRESH_WORD1, imxwd->base + WDOG_CNT);
	}

	return 0;
}

static enum wdog_hw_runnning imxulp_wd_running(struct imxulp_wd *imxwd)
{
	if (readl(imxwd->base + WDOG_CS) & WDOG_CS_EN)
		return WDOG_HW_RUNNING;
	else
		return WDOG_HW_NOT_RUNNING;
}

static int imxulp_wd_probe(struct device *dev)
{
	struct imxulp_wd *imxwd;
	struct resource *iores;
	struct imxulp_socdata *socdata;
	int ret;

	ret = dev_get_drvdata(dev, (const void **)&socdata);
	if (ret)
		return ret;

	imxwd = xzalloc(sizeof(*imxwd));
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return dev_err_probe(dev, PTR_ERR(iores), "could not get memory region\n");

	imxwd->rate = socdata->rate;
	imxwd->base = IOMEM(iores->start);
	imxwd->wd.set_timeout = imxulp_watchdog_set_timeout;
	imxwd->wd.timeout_max = 0xffff / imxwd->rate;
	imxwd->wd.hwdev = dev;
	imxwd->wd.running = imxulp_wd_running(imxwd);

	ret = watchdog_register(&imxwd->wd);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to register watchdog device\n");

	return 0;
}

static struct imxulp_socdata imx7ulp_wd_socdata = {
	.rate = CLK_RATE_1KHZ,
};

static struct imxulp_socdata imx93_wd_socdata = {
	.rate = CLK_RATE_32KHZ,
};

static __maybe_unused struct of_device_id imxulp_wdt_dt_ids[] = {
	{
		.compatible = "fsl,imx7ulp-wdt",
		.data = &imx7ulp_wd_socdata,
	}, {
		.compatible = "fsl,imx8ulp-wdt",
		.data = &imx7ulp_wd_socdata,
	}, {
		.compatible = "fsl,imx93-wdt",
		.data = &imx93_wd_socdata,
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, imx_wdt_dt_ids);

static struct driver imxulp_wd_driver = {
	.name = "imxulp-watchdog",
	.probe = imxulp_wd_probe,
	.of_compatible = DRV_OF_COMPAT(imxulp_wdt_dt_ids),
};
device_platform_driver(imxulp_wd_driver);
