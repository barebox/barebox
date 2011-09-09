/*
 * Atmel AT91 MCI driver
 *
 * Copyright (C) 2011 Hubert Feurstein <h.feurstein@gmail.com>
 *
 * based on imx.c by:
 * Copyright (C) 2009 Ilya Yanok, <yanok@emcraft.com>
 * Copyright (C) 2008 Sascha Hauer, Pengutronix <s.hauer@pengutronix.de>
 * Copyright (C) 2006 Pavel Pisa, PiKRON <ppisa@pikron.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <common.h>
#include <init.h>
#include <mci.h>
#include <errno.h>
#include <clock.h>
#include <gpio.h>
#include <asm/io.h>
#include <mach/board.h>
#include <linux/clk.h>
#include <linux/err.h>

#include "at91_mci.h"

struct atmel_mci_host {
	struct mci_host		mci;
	void  __iomem		*base;
	struct device_d		*hw_dev;
	struct clk		*clk;

	u32			datasize;
	struct mci_cmd		*cmd;
	struct mci_data		*data;
};

#define to_mci_host(mci)	container_of(mci, struct atmel_mci_host, mci)

#define STATUS_ERROR_MASK	(AT91_MCI_RINDE  \
				| AT91_MCI_RDIRE \
				| AT91_MCI_RCRCE \
				| AT91_MCI_RENDE \
				| AT91_MCI_RTOE  \
				| AT91_MCI_DCRCE \
				| AT91_MCI_DTOE  \
				| AT91_MCI_OVRE  \
				| AT91_MCI_UNRE)

static inline u32 atmel_mci_readl(struct atmel_mci_host *host, u32 offset)
{
	return readl(host->base + offset);
}

static inline void atmel_mci_writel(struct atmel_mci_host *host, u32 offset,
				    u32 value)
{
	writel(value, host->base + offset);
}

static void atmel_mci_reset(struct atmel_mci_host *host)
{
	atmel_mci_writel(host, AT91_MCI_CR, AT91_MCI_SWRST | AT91_MCI_MCIDIS);
	atmel_mci_writel(host, AT91_MCI_DTOR, 0x7f);
	atmel_mci_writel(host, AT91_MCI_IDR, ~0UL);
}

static void atmel_set_clk_rate(struct atmel_mci_host *host,
			       unsigned int clk_ios)
{
	unsigned int divider;
	unsigned int clk_in = clk_get_rate(host->clk);

	if (clk_ios > 0) {
		divider = (clk_in / clk_ios) / 2;
		if (divider > 0)
			divider -= 1;
	}

	if (clk_ios == 0 || divider > 255)
		divider = 255;

	dev_dbg(host->hw_dev, "atmel_set_clk_rate: clkIn=%d clkIos=%d divider=%d\n",
		clk_in, clk_ios, divider);

	atmel_mci_writel(host, AT91_MCI_MR, (AT91_MCI_CLKDIV & divider)
		| AT91_MCI_RDPROOF | AT91_MCI_WRPROOF);
}

static int atmel_poll_status(struct atmel_mci_host *host, u32 mask)
{
	u32 stat;
	uint64_t start = get_time_ns();

	do {
		stat = atmel_mci_readl(host, AT91_MCI_SR);
		if (stat & STATUS_ERROR_MASK)
			return stat;
		if (is_timeout(start, SECOND)) {
			dev_err(host->hw_dev, "timeout\n");
			return AT91_MCI_RTOE | stat;
		}
		if (stat & mask)
			return 0;
	} while (1);
}

static int atmel_pull(struct atmel_mci_host *host, void *_buf, int bytes)
{
	unsigned int stat;
	u32 *buf = _buf;

	while (bytes > 3) {
		stat = atmel_poll_status(host, AT91_MCI_RXRDY);
		if (stat)
			return stat;

		*buf++ = atmel_mci_readl(host, AT91_MCI_RDR);
		bytes -= 4;
	}

	if (WARN_ON(bytes))
		return -EIO;

	return 0;
}

#ifdef CONFIG_MCI_WRITE
static int atmel_push(struct atmel_mci_host *host, const void *_buf, int bytes)
{
	unsigned int stat;
	const u32 *buf = _buf;

	while (bytes > 3) {
		stat = atmel_poll_status(host, AT91_MCI_TXRDY);
		if (stat)
			return stat;

		atmel_mci_writel(host, AT91_MCI_TDR, *buf++);
		bytes -= 4;
	}

	stat = atmel_poll_status(host, AT91_MCI_TXRDY);
	if (stat)
		return stat;

	if (WARN_ON(bytes))
		return -EIO;

	return 0;
}
#endif /* CONFIG_MCI_WRITE */

static int atmel_transfer_data(struct atmel_mci_host *host)
{
	struct mci_data *data = host->data;
	int stat;
	unsigned long length;

	length = data->blocks * data->blocksize;
	host->datasize = 0;

	if (data->flags & MMC_DATA_READ) {
		stat = atmel_pull(host, data->dest, length);
		if (stat)
			return stat;

		stat = atmel_poll_status(host, AT91_MCI_NOTBUSY);
		if (stat)
			return stat;

		host->datasize += length;
	} else {
#ifdef CONFIG_MCI_WRITE
		stat = atmel_push(host, (const void *)(data->src), length);
		if (stat)
			return stat;

		host->datasize += length;
		stat = atmel_poll_status(host, AT91_MCI_NOTBUSY);
		if (stat)
			return stat;
#endif /* CONFIG_MCI_WRITE */
	}
	return 0;
}

static void atmel_finish_request(struct atmel_mci_host *host)
{
	host->cmd = NULL;
	host->data = NULL;
}

static int atmel_finish_data(struct atmel_mci_host *host, unsigned int stat)
{
	int data_error = 0;

	if (stat & STATUS_ERROR_MASK) {
		dev_err(host->hw_dev, "request failed (status=0x%08x)\n", stat);
		if (stat & AT91_MCI_DCRCE)
			data_error = -EILSEQ;
		else if (stat & (AT91_MCI_RTOE | AT91_MCI_DTOE))
			data_error = -ETIMEDOUT;
		else
			data_error = -EIO;
	}

	host->data = NULL;

	return data_error;
}

static void atmel_setup_data(struct atmel_mci_host *host, struct mci_data *data)
{
	unsigned int nob = data->blocks;
	unsigned int blksz = data->blocksize;
	unsigned int datasize = nob * blksz;

	BUG_ON(data->blocksize & 3);
	BUG_ON(nob == 0);

	host->data = data;

	dev_dbg(host->hw_dev, "atmel_setup_data: nob=%d blksz=%d\n",
		nob, blksz);

	atmel_mci_writel(host, AT91_MCI_BLKR, AT91_MCI_BLKR_BCNT(nob)
		| AT91_MCI_BLKR_BLKLEN(blksz));

	host->datasize = datasize;
}

static int atmel_read_response(struct atmel_mci_host *host, unsigned int stat)
{
	struct mci_cmd *cmd = host->cmd;
	int i;
	u32 *resp = (u32 *)cmd->response;

	if (!cmd)
		return 0;

	if (stat & (AT91_MCI_RTOE | AT91_MCI_DTOE)) {
		dev_err(host->hw_dev, "command/data timeout\n");
		return -ETIMEDOUT;
	} else if ((stat & AT91_MCI_RCRCE) && (cmd->resp_type & MMC_RSP_CRC)) {
		dev_err(host->hw_dev, "cmd crc error\n");
		return -EILSEQ;
	}

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		if (cmd->resp_type & MMC_RSP_136) {
			for (i = 0; i < 4; i++)
				resp[i] = atmel_mci_readl(host, AT91_MCI_RSPR(0));
		} else {
			resp[0] = atmel_mci_readl(host, AT91_MCI_RSPR(0));
		}
	}

	return 0;
}

static int atmel_cmd_done(struct atmel_mci_host *host, unsigned int stat)
{
	int datastat;
	int ret;

	ret = atmel_read_response(host, stat);

	if (ret) {
		atmel_finish_request(host);
		return ret;
	}

	if (!host->data) {
		atmel_finish_request(host);
		return 0;
	}

	datastat = atmel_transfer_data(host);
	ret = atmel_finish_data(host, datastat);
	atmel_finish_request(host);
	return ret;
}

static int atmel_start_cmd(struct atmel_mci_host *host, struct mci_cmd *cmd,
			   unsigned int cmdat)
{
	unsigned flags = 0;
	unsigned cmdval = 0;

	if (host->cmd != NULL)
		dev_err(host->hw_dev, "error!\n");

	if ((atmel_mci_readl(host, AT91_MCI_SR) & AT91_MCI_CMDRDY) == 0) {
		dev_err(host->hw_dev, "mci not ready!\n");
		return -EBUSY;
	}

	host->cmd = cmd;
	cmdval = AT91_MCI_CMDNB & cmd->cmdidx;

	switch (cmd->resp_type) {
	case MMC_RSP_R1: /* short CRC, OPCODE */
	case MMC_RSP_R1b:/* short CRC, OPCODE, BUSY */
		flags |= AT91_MCI_RSPTYP_48;
		break;
	case MMC_RSP_R2: /* long 136 bit + CRC */
		flags |= AT91_MCI_RSPTYP_136;
		break;
	case MMC_RSP_R3: /* short */
		flags |= AT91_MCI_RSPTYP_48;
		break;
	case MMC_RSP_NONE:
		flags |= AT91_MCI_RSPTYP_NONE;
		break;
	default:
		dev_err(host->hw_dev, "unhandled response type 0x%x\n",
				cmd->resp_type);
		return -EINVAL;
	}
	cmdval |= AT91_MCI_RSPTYP & flags;
	cmdval |= cmdat & ~(AT91_MCI_CMDNB | AT91_MCI_RSPTYP);

	atmel_mci_writel(host, AT91_MCI_ARGR, cmd->cmdarg);
	atmel_mci_writel(host, AT91_MCI_CMDR, cmdval);

	return 0;
}

/** init the host interface */
static int mci_reset(struct mci_host *mci, struct device_d *mci_dev)
{
	int ret;
	struct atmel_mci_host *host = to_mci_host(mci);
	struct atmel_mci_platform_data *pd = host->hw_dev->platform_data;

	ret = gpio_get_value(pd->detect_pin);
	dev_dbg(host->hw_dev, "card %sdetected\n", ret != 0 ? "not " : "");

	if (pd->detect_pin && ret == 1)
		return -ENODEV;

	clk_enable(host->clk);
	atmel_mci_reset(host);

	return 0;
}

/** change host interface settings */
static void mci_set_ios(struct mci_host *mci, struct device_d *mci_dev,
			unsigned bus_width, unsigned clock)
{
	struct atmel_mci_host *host = to_mci_host(mci);

	dev_dbg(host->hw_dev, "atmel_mci_set_ios: bus_width=%d clk=%d\n",
		bus_width, clock);

	switch (bus_width) {
	case 4:
		atmel_mci_writel(host, AT91_MCI_SDCR, AT91_MCI_SDCBUS_4BIT);
		break;
	case 8:
		atmel_mci_writel(host, AT91_MCI_SDCR, AT91_MCI_SDCBUS_8BIT);
		break;
	default:
		atmel_mci_writel(host, AT91_MCI_SDCR, AT91_MCI_SDCBUS_1BIT);
		break;
	}

	if (clock) {
		atmel_set_clk_rate(host, clock);
		atmel_mci_writel(host, AT91_MCI_CR, AT91_MCI_MCIEN
		);
	} else {
		atmel_mci_writel(host, AT91_MCI_CR, AT91_MCI_MCIDIS);
	}

	return;
}

/** handle a command */
static int mci_request(struct mci_host *mci, struct mci_cmd *cmd, struct mci_data *data)
{
	struct atmel_mci_host *host = to_mci_host(mci);
	u32 stat, cmdat = 0;
	int ret;

	if (cmd->resp_type != MMC_RSP_NONE)
		cmdat |= AT91_MCI_MAXLAT;

	if (data) {
		atmel_setup_data(host, data);

		cmdat |= AT91_MCI_TRCMD_START | AT91_MCI_TRTYP_MULTIPLE;

		if (data->flags & MMC_DATA_READ)
			cmdat |= AT91_MCI_TRDIR_RX;
	}

	ret = atmel_start_cmd(host, cmd, cmdat);
	if (ret) {
		atmel_finish_request(host);
		return ret;
	}

	stat = atmel_poll_status(host, AT91_MCI_CMDRDY);
	return atmel_cmd_done(host, stat);
}

#ifdef CONFIG_MCI_INFO
static void mci_info(struct device_d *mci_dev)
{
	struct atmel_mci_host *host = mci_dev->priv;
	struct atmel_mci_platform_data *pd = host->hw_dev->platform_data;

	printf("  Bus data width: %d bit\n", host->mci.bus_width);

	printf("  Bus frequency: %u Hz\n", host->mci.clock);
	printf("  Frequency limits: ");
	if (host->mci.f_min == 0)
		printf("no lower limit ");
	else
		printf("%u Hz lower limit ", host->mci.f_min);
	if (host->mci.f_max == 0)
		printf("- no upper limit");
	else
		printf("- %u Hz upper limit", host->mci.f_max);

	printf("\n  Card detection support: %s\n",
		pd->detect_pin != 0 ? "yes" : "no");

}
#endif /* CONFIG_MCI_INFO */

static int mci_probe(struct device_d *hw_dev)
{
	unsigned long clk_rate;
	struct atmel_mci_host *host;
	struct atmel_mci_platform_data *pd = hw_dev->platform_data;

	if (!pd) {
		dev_err(hw_dev, "missing platform data\n");
		return -EINVAL;
	}

	host = xzalloc(sizeof(*host));
	host->mci.send_cmd = mci_request;
	host->mci.set_ios = mci_set_ios;
	host->mci.init = mci_reset;
	host->mci.hw_dev = hw_dev;

	host->mci.host_caps = pd->host_caps;
	if (pd->bus_width >= 4)
		host->mci.host_caps |= MMC_MODE_4BIT;
	if (pd->bus_width == 8)
		host->mci.host_caps |= MMC_MODE_8BIT;

	host->base = dev_request_mem_region(hw_dev, 0);
	host->hw_dev = hw_dev;
	hw_dev->priv = host;
	host->clk = clk_get(hw_dev, "mci_clk");
	if (IS_ERR(host->clk)) {
		dev_err(hw_dev, "no mci_clk\n");
		return PTR_ERR(host->clk);
	}

	clk_rate = clk_get_rate(host->clk);

	host->mci.voltages = MMC_VDD_32_33 | MMC_VDD_33_34;

	host->mci.f_min = clk_rate >> 9;
	host->mci.f_max = clk_rate >> 1;

	mci_register(&host->mci);

	return 0;
}

static struct driver_d atmel_mci_driver = {
	.name	= "atmel_mci",
	.probe	= mci_probe,
#ifdef CONFIG_MCI_INFO
	.info	= mci_info,
#endif
};

static int atmel_mci_init_driver(void)
{
	register_driver(&atmel_mci_driver);
	return 0;
}

device_initcall(atmel_mci_init_driver);
