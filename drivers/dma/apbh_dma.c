// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Freescale i.MX28 APBH DMA driver
 *
 * Copyright (C) 2011 Wolfram Sang <w.sang@pengutronix.de>
 *
 * Copyright (C) 2011 Marek Vasut <marek.vasut@gmail.com>
 * on behalf of DENX Software Engineering GmbH
 *
 * Based on code from LTIB:
 * Copyright (C) 2010 Freescale Semiconductor, Inc. All Rights Reserved.
 */

#include <dma/apbh-dma.h>
#include <stmp-device.h>
#include <linux/clk.h>
#include <linux/list.h>
#include <linux/err.h>
#include <common.h>
#include <dma.h>
#include <driver.h>
#include <malloc.h>
#include <errno.h>
#include <init.h>
#include <io.h>

struct apbh_dma {
	void __iomem *regs;
	struct clk *clk;
	enum mxs_dma_id id;
};

static struct apbh_dma *apbh_dma;

/*
 * Test is the DMA channel is valid channel
 */
static int mxs_dma_validate_chan(int channel)
{
	if ((channel < 0) || (channel >= MXS_MAX_DMA_CHANNELS))
		return -EINVAL;

	return 0;
}

/*
 * Resets the DMA channel hardware.
 */
static int mxs_dma_reset(int channel)
{
	struct apbh_dma *apbh = apbh_dma;

	if (apbh_dma_is_imx23(apbh))
		writel(1 << (channel + BP_APBH_CTRL0_RESET_CHANNEL),
			apbh->regs + HW_APBHX_CTRL0 + STMP_OFFSET_REG_SET);
	else
		writel(1 << (channel + BP_APBHX_CHANNEL_CTRL_RESET_CHANNEL),
			apbh->regs + HW_APBHX_CHANNEL_CTRL + STMP_OFFSET_REG_SET);

	return 0;
}

/*
 * Clear DMA interrupt.
 *
 * The software that is using the DMA channel must register to receive its
 * interrupts and, when they arrive, must call this function to clear them.
 */
static int mxs_dma_ack_irq(int channel)
{
	struct apbh_dma *apbh = apbh_dma;

	writel(1 << channel, apbh->regs + HW_APBHX_CTRL1 + STMP_OFFSET_REG_CLR);
	writel(1 << channel, apbh->regs + HW_APBHX_CTRL2 + STMP_OFFSET_REG_CLR);

	return 0;
}

/*
 * Wait for DMA channel to complete
 */
static int mxs_dma_wait_complete(uint32_t timeout, unsigned int chan)
{
	struct apbh_dma *apbh = apbh_dma;

	while (--timeout) {
		if (readl(apbh->regs + HW_APBHX_CTRL1) & (1 << chan))
			break;
		udelay(1);
	}

	if (!timeout)
		return -ETIMEDOUT;

	return 0;
}

/*
 * Execute the DMA channel
 */
int mxs_dma_go(int chan, struct mxs_dma_cmd *cmd, int ncmds)
{
	struct apbh_dma *apbh = apbh_dma;
	uint32_t timeout = 10000;
	int i, ret, channel_bit;

	ret = mxs_dma_validate_chan(chan);
	if (ret)
		return ret;

	for (i = 0; i < ncmds - 1; i++) {
		cmd[i].next = (unsigned long)(&cmd[i + 1]);
		cmd[i].data |= MXS_DMA_DESC_CHAIN;
	}

	if (apbh_dma_is_imx23(apbh)) {
		writel(cmd, apbh->regs + HW_APBHX_CHn_NXTCMDAR_MX23(chan));
		writel(1, apbh->regs + HW_APBHX_CHn_SEMA_MX23(chan));
		channel_bit = chan + BP_APBH_CTRL0_CLKGATE_CHANNEL;
	} else {
		writel(cmd, apbh->regs + HW_APBHX_CHn_NXTCMDAR_MX28(chan));
		writel(1, apbh->regs + HW_APBHX_CHn_SEMA_MX28(chan));
		channel_bit = chan;
	}
	writel(1 << channel_bit, apbh->regs + HW_APBHX_CTRL0 + STMP_OFFSET_REG_CLR);

	/* Wait for DMA to finish. */
	ret = mxs_dma_wait_complete(timeout, chan);

	/* Shut the DMA channel down. */
	mxs_dma_ack_irq(chan);
	mxs_dma_reset(chan);

	return ret;
}

/*
 * Initialize the DMA hardware
 */
static int apbh_dma_probe(struct device *dev)
{
	struct resource *iores;
	struct apbh_dma *apbh;
	enum mxs_dma_id id;
	int ret, channel;

	id = (enum mxs_dma_id)device_get_match_data(dev);
	if (id == UNKNOWN_DMA_ID)
		return -ENODEV;

	apbh_dma = apbh = xzalloc(sizeof(*apbh));
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	apbh->regs = IOMEM(iores->start);

	apbh->id = id;

	apbh->clk = clk_get(dev, NULL);
	if (IS_ERR(apbh->clk))
		return PTR_ERR(apbh->clk);

	ret = clk_enable(apbh->clk);
	if (ret) {
		dev_err(dev, "Failed to enable clock: %s\n",
			strerror(ret));
		return ret;
	}

	ret = stmp_reset_block(apbh->regs, 0);
	if (ret)
		return ret;

	writel(BM_APBH_CTRL0_APB_BURST8_EN,
		apbh->regs + HW_APBHX_CTRL0 + STMP_OFFSET_REG_SET);

	writel(BM_APBH_CTRL0_APB_BURST_EN,
		apbh->regs + HW_APBHX_CTRL0 + STMP_OFFSET_REG_SET);

	for (channel = 0; channel < MXS_MAX_DMA_CHANNELS; channel++) {
		mxs_dma_reset(channel);
		mxs_dma_ack_irq(channel);
	}

	return 0;
}

static struct platform_device_id apbh_ids[] = {
	{
		.name = "imx23-dma-apbh",
		.driver_data = (unsigned long)IMX23_DMA,
        }, {
		.name = "imx28-dma-apbh",
		.driver_data = (unsigned long)IMX28_DMA,
        }, {
		/* sentinel */
	}
};

static __maybe_unused struct of_device_id apbh_dt_ids[] = {
	{
		.compatible = "fsl,imx23-dma-apbh",
		.data = (void *)IMX23_DMA,
	}, {
		.compatible = "fsl,imx28-dma-apbh",
		.data = (void *)IMX28_DMA,
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, apbh_dt_ids);

static struct driver apbh_dma_driver = {
	.name  = "dma-apbh",
	.id_table = apbh_ids,
	.of_compatible = DRV_OF_COMPAT(apbh_dt_ids),
	.probe = apbh_dma_probe,
};
coredevice_platform_driver(apbh_dma_driver);
