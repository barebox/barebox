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
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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


#define HW_APBHX_CTRL0				0x000
#define BM_APBH_CTRL0_APB_BURST8_EN		(1 << 29)
#define BM_APBH_CTRL0_APB_BURST_EN		(1 << 28)
#define BP_APBH_CTRL0_CLKGATE_CHANNEL		8
#define BP_APBH_CTRL0_RESET_CHANNEL		16
#define HW_APBHX_CTRL1				0x010
#define	BP_APBHX_CTRL1_CH_CMDCMPLT_IRQ_EN	16
#define HW_APBHX_CTRL2				0x020
#define HW_APBHX_CHANNEL_CTRL			0x030
#define BP_APBHX_CHANNEL_CTRL_RESET_CHANNEL	16
#define BP_APBHX_VERSION_MAJOR			24
#define HW_APBHX_CHn_NXTCMDAR_MX23(n)		(0x050 + (n) * 0x70)
#define HW_APBHX_CHn_NXTCMDAR_MX28(n)		(0x110 + (n) * 0x70)
#define HW_APBHX_CHn_SEMA_MX23(n)		(0x080 + (n) * 0x70)
#define HW_APBHX_CHn_SEMA_MX28(n)		(0x140 + (n) * 0x70)
#define	BM_APBHX_CHn_SEMA_PHORE			(0xff << 16)
#define	BP_APBHX_CHn_SEMA_PHORE			16

static struct mxs_dma_chan mxs_dma_channels[MXS_MAX_DMA_CHANNELS];

enum mxs_dma_id {
	IMX23_DMA,
	IMX28_DMA,
};

struct apbh_dma {
	void __iomem *regs;
	struct clk *clk;
	enum mxs_dma_id id;
};

#define apbh_dma_is_imx23(aphb) ((apbh)->id == IMX23_DMA)

static struct apbh_dma *apbh_dma;

/*
 * Test is the DMA channel is valid channel
 */
int mxs_dma_validate_chan(int channel)
{
	struct mxs_dma_chan *pchan;

	if ((channel < 0) || (channel >= MXS_MAX_DMA_CHANNELS))
		return -EINVAL;

	pchan = mxs_dma_channels + channel;
	if (!(pchan->flags & MXS_DMA_FLAGS_ALLOCATED))
		return -EINVAL;

	return 0;
}

/*
 * Return the address of the command within a descriptor.
 */
static unsigned int mxs_dma_cmd_address(struct mxs_dma_desc *desc)
{
	return desc->address + offsetof(struct mxs_dma_desc, cmd);
}

/*
 * Read a DMA channel's hardware semaphore.
 *
 * As used by the MXS platform's DMA software, the DMA channel's hardware
 * semaphore reflects the number of DMA commands the hardware will process, but
 * has not yet finished. This is a volatile value read directly from hardware,
 * so it must be be viewed as immediately stale.
 *
 * If the channel is not marked busy, or has finished processing all its
 * commands, this value should be zero.
 *
 * See mxs_dma_append() for details on how DMA command blocks must be configured
 * to maintain the expected behavior of the semaphore's value.
 */
static int mxs_dma_read_semaphore(int channel)
{
	struct apbh_dma *apbh = apbh_dma;
	uint32_t tmp;
	int ret;

	ret = mxs_dma_validate_chan(channel);
	if (ret)
		return ret;

	if (apbh_dma_is_imx23(apbh))
		tmp = readl(apbh->regs + HW_APBHX_CHn_SEMA_MX23(channel));
	else
		tmp = readl(apbh->regs + HW_APBHX_CHn_SEMA_MX28(channel));

	tmp &= BM_APBHX_CHn_SEMA_PHORE;
	tmp >>= BP_APBHX_CHn_SEMA_PHORE;

	return tmp;
}

/*
 * Enable a DMA channel.
 *
 * If the given channel has any DMA descriptors on its active list, this
 * function causes the DMA hardware to begin processing them.
 *
 * This function marks the DMA channel as "busy," whether or not there are any
 * descriptors to process.
 */
static int mxs_dma_enable(int channel)
{
	struct apbh_dma *apbh = apbh_dma;
	unsigned int sem;
	struct mxs_dma_chan *pchan;
	struct mxs_dma_desc *pdesc;
	int channel_bit, ret;

	ret = mxs_dma_validate_chan(channel);
	if (ret)
		return ret;

	pchan = mxs_dma_channels + channel;

	if (pchan->pending_num == 0) {
		pchan->flags |= MXS_DMA_FLAGS_BUSY;
		return 0;
	}

	pdesc = list_first_entry(&pchan->active, struct mxs_dma_desc, node);
	if (pdesc == NULL)
		return -EFAULT;

	if (pchan->flags & MXS_DMA_FLAGS_BUSY) {
		if (!(pdesc->cmd.data & MXS_DMA_DESC_CHAIN))
			return 0;

		sem = mxs_dma_read_semaphore(channel);
		if (sem == 0)
			return 0;

		if (sem == 1) {
			pdesc = list_entry(pdesc->node.next,
					   struct mxs_dma_desc, node);
			if (apbh_dma_is_imx23(apbh))
				writel(mxs_dma_cmd_address(pdesc),
					apbh->regs + HW_APBHX_CHn_NXTCMDAR_MX23(channel));
			else
				writel(mxs_dma_cmd_address(pdesc),
					apbh->regs + HW_APBHX_CHn_NXTCMDAR_MX28(channel));
		}

		if (apbh_dma_is_imx23(apbh))
			writel(pchan->pending_num,
					apbh->regs + HW_APBHX_CHn_SEMA_MX23(channel));
		else
			writel(pchan->pending_num,
					apbh->regs + HW_APBHX_CHn_SEMA_MX28(channel));

		pchan->active_num += pchan->pending_num;
		pchan->pending_num = 0;
	} else {
		pchan->active_num += pchan->pending_num;
		pchan->pending_num = 0;
		if (apbh_dma_is_imx23(apbh)) {
			writel(mxs_dma_cmd_address(pdesc),
				apbh->regs + HW_APBHX_CHn_NXTCMDAR_MX23(channel));
			writel(pchan->active_num,
				apbh->regs + HW_APBHX_CHn_SEMA_MX23(channel));
			channel_bit = channel + BP_APBH_CTRL0_CLKGATE_CHANNEL;
		} else {
			writel(mxs_dma_cmd_address(pdesc),
				apbh->regs + HW_APBHX_CHn_NXTCMDAR_MX28(channel));
			writel(pchan->active_num,
				apbh->regs + HW_APBHX_CHn_SEMA_MX28(channel));
			channel_bit = channel;
		}
		writel(1 << channel_bit, apbh->regs + HW_APBHX_CTRL0 + STMP_OFFSET_REG_CLR);
	}

	pchan->flags |= MXS_DMA_FLAGS_BUSY;
	return 0;
}

/*
 * Disable a DMA channel.
 *
 * This function shuts down a DMA channel and marks it as "not busy." Any
 * descriptors on the active list are immediately moved to the head of the
 * "done" list, whether or not they have actually been processed by the
 * hardware. The "ready" flags of these descriptors are NOT cleared, so they
 * still appear to be active.
 *
 * This function immediately shuts down a DMA channel's hardware, aborting any
 * I/O that may be in progress, potentially leaving I/O hardware in an undefined
 * state. It is unwise to call this function if there is ANY chance the hardware
 * is still processing a command.
 */
static int mxs_dma_disable(int channel)
{
	struct mxs_dma_chan *pchan;
	struct apbh_dma *apbh = apbh_dma;
	int channel_bit, ret;

	ret = mxs_dma_validate_chan(channel);
	if (ret)
		return ret;

	pchan = mxs_dma_channels + channel;

	if (!(pchan->flags & MXS_DMA_FLAGS_BUSY))
		return -EINVAL;

	if (apbh_dma_is_imx23(apbh))
		channel_bit = channel + BP_APBH_CTRL0_CLKGATE_CHANNEL;
	else
		channel_bit = channel + 0;

	writel(1 << channel_bit, apbh->regs + HW_APBHX_CTRL0 + STMP_OFFSET_REG_SET);

	pchan->flags &= ~MXS_DMA_FLAGS_BUSY;
	pchan->active_num = 0;
	pchan->pending_num = 0;
	list_splice_init(&pchan->active, &pchan->done);

	return 0;
}

/*
 * Resets the DMA channel hardware.
 */
static int mxs_dma_reset(int channel)
{
	struct apbh_dma *apbh = apbh_dma;
	int ret;

	ret = mxs_dma_validate_chan(channel);
	if (ret)
		return ret;

	if (apbh_dma_is_imx23(apbh))
		writel(1 << (channel + BP_APBH_CTRL0_RESET_CHANNEL),
			apbh->regs + HW_APBHX_CTRL0 + STMP_OFFSET_REG_SET);
	else
		writel(1 << (channel + BP_APBHX_CHANNEL_CTRL_RESET_CHANNEL),
			apbh->regs + HW_APBHX_CHANNEL_CTRL + STMP_OFFSET_REG_SET);

	return 0;
}

/*
 * Enable or disable DMA interrupt.
 *
 * This function enables the given DMA channel to interrupt the CPU.
 */
static int mxs_dma_enable_irq(int channel, int enable)
{
	struct apbh_dma *apbh = apbh_dma;
	int ret;

	ret = mxs_dma_validate_chan(channel);
	if (ret)
		return ret;

	if (enable)
		writel(1 << (channel + BP_APBHX_CTRL1_CH_CMDCMPLT_IRQ_EN),
			apbh->regs + HW_APBHX_CTRL1 + STMP_OFFSET_REG_SET);
	else
		writel(1 << (channel + BP_APBHX_CTRL1_CH_CMDCMPLT_IRQ_EN),
			apbh->regs + HW_APBHX_CTRL1 + STMP_OFFSET_REG_CLR);

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
	int ret;

	ret = mxs_dma_validate_chan(channel);
	if (ret)
		return ret;

	writel(1 << channel, apbh->regs + HW_APBHX_CTRL1 + STMP_OFFSET_REG_CLR);
	writel(1 << channel, apbh->regs + HW_APBHX_CTRL2 + STMP_OFFSET_REG_CLR);

	return 0;
}

/*
 * Request to reserve a DMA channel
 */
static int mxs_dma_request(int channel)
{
	struct mxs_dma_chan *pchan;

	if ((channel < 0) || (channel >= MXS_MAX_DMA_CHANNELS))
		return -EINVAL;

	pchan = mxs_dma_channels + channel;
	if ((pchan->flags & MXS_DMA_FLAGS_VALID) != MXS_DMA_FLAGS_VALID)
		return -ENODEV;

	if (pchan->flags & MXS_DMA_FLAGS_ALLOCATED)
		return -EBUSY;

	pchan->flags |= MXS_DMA_FLAGS_ALLOCATED;
	pchan->active_num = 0;
	pchan->pending_num = 0;

	INIT_LIST_HEAD(&pchan->active);
	INIT_LIST_HEAD(&pchan->done);

	return 0;
}

/*
 * Release a DMA channel.
 *
 * This function releases a DMA channel from its current owner.
 *
 * The channel will NOT be released if it's marked "busy" (see
 * mxs_dma_enable()).
 */
static int mxs_dma_release(int channel)
{
	struct mxs_dma_chan *pchan;
	int ret;

	ret = mxs_dma_validate_chan(channel);
	if (ret)
		return ret;

	pchan = mxs_dma_channels + channel;

	if (pchan->flags & MXS_DMA_FLAGS_BUSY)
		return -EBUSY;

	pchan->dev = 0;
	pchan->active_num = 0;
	pchan->pending_num = 0;
	pchan->flags &= ~MXS_DMA_FLAGS_ALLOCATED;

	return 0;
}

/*
 * Allocate DMA descriptor
 */
struct mxs_dma_desc *mxs_dma_desc_alloc(void)
{
	struct mxs_dma_desc *pdesc;

	pdesc = dma_alloc_coherent(sizeof(struct mxs_dma_desc),
				   DMA_ADDRESS_BROKEN);

	if (pdesc == NULL)
		return NULL;

	memset(pdesc, 0, sizeof(*pdesc));
	pdesc->address = (dma_addr_t)pdesc;

	return pdesc;
};

/*
 * Free DMA descriptor
 */
void mxs_dma_desc_free(struct mxs_dma_desc *pdesc)
{
	if (pdesc == NULL)
		return;

	free(pdesc);
}

/*
 * Add a DMA descriptor to a channel.
 *
 * If the descriptor list for this channel is not empty, this function sets the
 * CHAIN bit and the NEXTCMD_ADDR fields in the last descriptor's DMA command so
 * it will chain to the new descriptor's command.
 *
 * Then, this function marks the new descriptor as "ready," adds it to the end
 * of the active descriptor list, and increments the count of pending
 * descriptors.
 *
 * The MXS platform DMA software imposes some rules on DMA commands to maintain
 * important invariants. These rules are NOT checked, but they must be carefully
 * applied by software that uses MXS DMA channels.
 *
 * Invariant:
 *     The DMA channel's hardware semaphore must reflect the number of DMA
 *     commands the hardware will process, but has not yet finished.
 *
 * Explanation:
 *     A DMA channel begins processing commands when its hardware semaphore is
 *     written with a value greater than zero, and it stops processing commands
 *     when the semaphore returns to zero.
 *
 *     When a channel finishes a DMA command, it will decrement its semaphore if
 *     the DECREMENT_SEMAPHORE bit is set in that command's flags bits.
 *
 *     In principle, it's not necessary for the DECREMENT_SEMAPHORE to be set,
 *     unless it suits the purposes of the software. For example, one could
 *     construct a series of five DMA commands, with the DECREMENT_SEMAPHORE
 *     bit set only in the last one. Then, setting the DMA channel's hardware
 *     semaphore to one would cause the entire series of five commands to be
 *     processed. However, this example would violate the invariant given above.
 *
 * Rule:
 *    ALL DMA commands MUST have the DECREMENT_SEMAPHORE bit set so that the DMA
 *    channel's hardware semaphore will be decremented EVERY time a command is
 *    processed.
 */
int mxs_dma_desc_append(int channel, struct mxs_dma_desc *pdesc)
{
	struct mxs_dma_chan *pchan;
	struct mxs_dma_desc *last;
	int ret;

	ret = mxs_dma_validate_chan(channel);
	if (ret)
		return ret;

	pchan = mxs_dma_channels + channel;

	pdesc->cmd.next = mxs_dma_cmd_address(pdesc);
	pdesc->flags |= MXS_DMA_DESC_FIRST | MXS_DMA_DESC_LAST;

	if (!list_empty(&pchan->active)) {
		last = list_entry(pchan->active.prev, struct mxs_dma_desc,
					node);

		pdesc->flags &= ~MXS_DMA_DESC_FIRST;
		last->flags &= ~MXS_DMA_DESC_LAST;

		last->cmd.next = mxs_dma_cmd_address(pdesc);
		last->cmd.data |= MXS_DMA_DESC_CHAIN;
	}
	pdesc->flags |= MXS_DMA_DESC_READY;
	if (pdesc->flags & MXS_DMA_DESC_FIRST)
		pchan->pending_num++;
	list_add_tail(&pdesc->node, &pchan->active);

	return ret;
}

/*
 * Clean up processed DMA descriptors.
 *
 * This function removes processed DMA descriptors from the "active" list. Pass
 * in a non-NULL list head to get the descriptors moved to your list. Pass NULL
 * to get the descriptors moved to the channel's "done" list. Descriptors on
 * the "done" list can be retrieved with mxs_dma_get_finished().
 *
 * This function marks the DMA channel as "not busy" if no unprocessed
 * descriptors remain on the "active" list.
 */
static int mxs_dma_finish(int channel, struct list_head *head)
{
	int sem;
	struct mxs_dma_chan *pchan;
	struct list_head *p, *q;
	struct mxs_dma_desc *pdesc;
	int ret;

	ret = mxs_dma_validate_chan(channel);
	if (ret)
		return ret;

	pchan = mxs_dma_channels + channel;

	sem = mxs_dma_read_semaphore(channel);
	if (sem < 0)
		return sem;

	if (sem == pchan->active_num)
		return 0;

	list_for_each_safe(p, q, &pchan->active) {
		if ((pchan->active_num) <= sem)
			break;

		pdesc = list_entry(p, struct mxs_dma_desc, node);
		pdesc->flags &= ~MXS_DMA_DESC_READY;

		if (head)
			list_move_tail(p, head);
		else
			list_move_tail(p, &pchan->done);

		if (pdesc->flags & MXS_DMA_DESC_LAST)
			pchan->active_num--;
	}

	if (sem == 0)
		pchan->flags &= ~MXS_DMA_FLAGS_BUSY;

	return 0;
}

/*
 * Wait for DMA channel to complete
 */
static int mxs_dma_wait_complete(uint32_t timeout, unsigned int chan)
{
	struct apbh_dma *apbh = apbh_dma;
	int ret;

	ret = mxs_dma_validate_chan(chan);
	if (ret)
		return ret;

	while (--timeout) {
		if (readl(apbh->regs + HW_APBHX_CTRL1) & (1 << chan))
			break;
		udelay(1);
	}

	if (timeout == 0) {
		ret = -ETIMEDOUT;
		mxs_dma_reset(chan);
	}

	return ret;
}

/*
 * Execute the DMA channel
 */
int mxs_dma_go(int chan)
{
	uint32_t timeout = 10000;
	int ret;

	LIST_HEAD(tmp_desc_list);

	mxs_dma_enable_irq(chan, 1);
	mxs_dma_enable(chan);

	/* Wait for DMA to finish. */
	ret = mxs_dma_wait_complete(timeout, chan);

	/* Clear out the descriptors we just ran. */
	mxs_dma_finish(chan, &tmp_desc_list);

	/* Shut the DMA channel down. */
	mxs_dma_ack_irq(chan);
	mxs_dma_reset(chan);
	mxs_dma_enable_irq(chan, 0);
	mxs_dma_disable(chan);

	return ret;
}

/*
 * Initialize the DMA hardware
 */
static int apbh_dma_probe(struct device_d *dev)
{
	struct resource *iores;
	struct apbh_dma *apbh;
	struct mxs_dma_chan *pchan;
	enum mxs_dma_id id;
	int ret, channel;

	ret = dev_get_drvdata(dev, (const void **)&id);
	if (ret)
		return ret;

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
		pchan = mxs_dma_channels + channel;
		pchan->flags = MXS_DMA_FLAGS_VALID;

		ret = mxs_dma_request(channel);

		if (ret) {
			printf("MXS DMA: Can't acquire DMA channel %i\n",
				channel);

			goto err;
		}

		mxs_dma_reset(channel);
		mxs_dma_ack_irq(channel);
	}

	return 0;

err:
	while (--channel >= 0)
		mxs_dma_release(channel);
	return ret;
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

static struct driver_d apbh_dma_driver = {
	.name  = "dma-apbh",
	.id_table = apbh_ids,
	.of_compatible = DRV_OF_COMPAT(apbh_dt_ids),
	.probe = apbh_dma_probe,
};
coredevice_platform_driver(apbh_dma_driver);
