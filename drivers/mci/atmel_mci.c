/*
 * Atmel MCI driver
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
#include <io.h>
#include <mach/board.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <of_gpio.h>

#include "atmel-mci-regs.h"

struct atmel_mci_caps {
	bool	has_cfg_reg;
	bool	has_highspeed;
	bool    has_rwproof;
	bool	has_odd_clk_div;
	bool	need_reset_after_xfer;
};

struct atmel_mci {
	struct mci_host		mci;
	void  __iomem		*regs;
	struct device_d		*hw_dev;
	struct clk		*clk;

	u32			datasize;
	struct mci_cmd		*cmd;
	struct mci_data		*data;
	unsigned		slot_b;
	int			version;
	struct atmel_mci_caps	caps;

	unsigned long		bus_hz;
	u32			mode_reg;
	u32			cfg_reg;
	u32			sdc_reg;
	bool			need_reset;
	int			detect_pin;
};

#define to_mci_host(mci)	container_of(mci, struct atmel_mci, mci)

#define STATUS_ERROR_MASK	(ATMCI_RINDE  \
				| ATMCI_RDIRE \
				| ATMCI_RCRCE \
				| ATMCI_RENDE \
				| ATMCI_RTOE  \
				| ATMCI_DCRCE \
				| ATMCI_DTOE  \
				| ATMCI_OVRE  \
				| ATMCI_UNRE)

static void atmci_set_clk_rate(struct atmel_mci *host,
			       unsigned int clock_min)
{
	unsigned int clkdiv;

	if (!host->mode_reg) {
		clk_enable(host->clk);
		atmci_writel(host, ATMCI_CR, ATMCI_CR_MCIEN);
		if (host->caps.has_cfg_reg)
			atmci_writel(host, ATMCI_CFG, host->cfg_reg);
	}

	if (host->caps.has_odd_clk_div) {
		clkdiv = DIV_ROUND_UP(host->bus_hz, clock_min) - 2;
		if (clkdiv > 511) {
			dev_dbg(host->hw_dev,
			         "clock %u too slow; using %lu\n",
			         clock_min, host->bus_hz / (511 + 2));
			clkdiv = 511;
		}
		host->mode_reg = ATMCI_MR_CLKDIV(clkdiv >> 1)
				 | ATMCI_MR_CLKODD(clkdiv & 1);
	} else {
		clkdiv = DIV_ROUND_UP(host->bus_hz, 2 * clock_min) - 1;
		if (clkdiv > 255) {
			dev_dbg(host->hw_dev,
				 "clock %u too slow; using %lu\n",
				 clock_min, host->bus_hz / (2 * 256));
			clkdiv = 255;
		}
		host->mode_reg = ATMCI_MR_CLKDIV(clkdiv);
	}

	dev_dbg(host->hw_dev, "atmel_set_clk_rate: clkIn=%ld clkIos=%d divider=%d\n",
		host->bus_hz, clock_min, clkdiv);

	/*
	 * WRPROOF and RDPROOF prevent overruns/underruns by
	 * stopping the clock when the FIFO is full/empty.
	 * This state is not expected to last for long.
	 */
	if (host->caps.has_rwproof)
		host->mode_reg |= ATMCI_MR_RDPROOF | ATMCI_MR_WRPROOF;

	atmci_writel(host, ATMCI_MR, host->mode_reg);
}

static int atmci_poll_status(struct atmel_mci *host, u32 mask)
{
	u32 stat;
	uint64_t start = get_time_ns();

	do {
		stat = atmci_readl(host, ATMCI_SR);
		if (stat & STATUS_ERROR_MASK)
			return stat;
		if (is_timeout(start, SECOND)) {
			dev_err(host->hw_dev, "timeout\n");
			host->need_reset = true;
			return ATMCI_RTOE | stat;
		}
		if (stat & mask)
			return 0;
	} while (1);
}

static int atmci_pull(struct atmel_mci *host, void *_buf, int bytes)
{
	unsigned int stat;
	u32 *buf = _buf;

	while (bytes > 3) {
		stat = atmci_poll_status(host, ATMCI_RXRDY);
		if (stat)
			return stat;

		*buf++ = atmci_readl(host, ATMCI_RDR);
		bytes -= 4;
	}

	if (WARN_ON(bytes))
		return -EIO;

	return 0;
}

#ifdef CONFIG_MCI_WRITE
static int atmci_push(struct atmel_mci *host, const void *_buf, int bytes)
{
	unsigned int stat;
	const u32 *buf = _buf;

	while (bytes > 3) {
		stat = atmci_poll_status(host, ATMCI_TXRDY);
		if (stat)
			return stat;

		atmci_writel(host, ATMCI_TDR, *buf++);
		bytes -= 4;
	}

	stat = atmci_poll_status(host, ATMCI_TXRDY);
	if (stat)
		return stat;

	if (WARN_ON(bytes))
		return -EIO;

	return 0;
}
#endif /* CONFIG_MCI_WRITE */

static int atmci_transfer_data(struct atmel_mci *host)
{
	struct mci_data *data = host->data;
	int stat;
	unsigned long length;

	length = data->blocks * data->blocksize;
	host->datasize = 0;

	if (data->flags & MMC_DATA_READ) {
		stat = atmci_pull(host, data->dest, length);
		if (stat)
			return stat;

		stat = atmci_poll_status(host, ATMCI_NOTBUSY);
		if (stat)
			return stat;

		host->datasize += length;
	} else {
#ifdef CONFIG_MCI_WRITE
		stat = atmci_push(host, (const void *)(data->src), length);
		if (stat)
			return stat;

		host->datasize += length;
		stat = atmci_poll_status(host, ATMCI_NOTBUSY);
		if (stat)
			return stat;
#endif /* CONFIG_MCI_WRITE */
	}
	return 0;
}

static void atmci_finish_request(struct atmel_mci *host)
{
	host->cmd = NULL;
	host->data = NULL;
}

static int atmci_finish_data(struct atmel_mci *host, unsigned int stat)
{
	int data_error = 0;

	if (stat & STATUS_ERROR_MASK) {
		dev_err(host->hw_dev, "request failed (status=0x%08x)\n", stat);
		if (stat & ATMCI_DCRCE)
			data_error = -EILSEQ;
		else if (stat & (ATMCI_RTOE | ATMCI_DTOE))
			data_error = -ETIMEDOUT;
		else
			data_error = -EIO;
	}

	host->data = NULL;

	return data_error;
}

static void atmci_setup_data(struct atmel_mci *host, struct mci_data *data)
{
	unsigned int nob = data->blocks;
	unsigned int blksz = data->blocksize;
	unsigned int datasize = nob * blksz;

	BUG_ON(data->blocksize & 3);
	BUG_ON(nob == 0);

	host->data = data;

	dev_dbg(host->hw_dev, "atmel_setup_data: nob=%d blksz=%d\n",
		nob, blksz);

	atmci_writel(host, ATMCI_BLKR, ATMCI_BCNT(nob)
		| ATMCI_BLKLEN(blksz));

	host->datasize = datasize;
}

static int atmci_read_response(struct atmel_mci *host, unsigned int stat)
{
	struct mci_cmd *cmd = host->cmd;
	int i;
	u32 *resp;

	if (!cmd)
		return 0;

	resp = (u32 *)cmd->response;

	if (stat & (ATMCI_RTOE | ATMCI_DTOE)) {
		dev_err(host->hw_dev, "command/data timeout\n");
		return -ETIMEDOUT;
	} else if ((stat & ATMCI_RCRCE) && (cmd->resp_type & MMC_RSP_CRC)) {
		dev_err(host->hw_dev, "cmd crc error\n");
		return -EILSEQ;
	}

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		if (cmd->resp_type & MMC_RSP_136) {
			for (i = 0; i < 4; i++)
				resp[i] = atmci_readl(host, ATMCI_RSPR);
		} else {
			resp[0] = atmci_readl(host, ATMCI_RSPR);
		}
	}

	return 0;
}

static int atmci_cmd_done(struct atmel_mci *host, unsigned int stat)
{
	int datastat;
	int ret;

	ret = atmci_read_response(host, stat);

	if (ret) {
		atmci_finish_request(host);
		return ret;
	}

	if (!host->data) {
		atmci_finish_request(host);
		return 0;
	}

	datastat = atmci_transfer_data(host);
	ret = atmci_finish_data(host, datastat);
	atmci_finish_request(host);
	return ret;
}

static int atmci_start_cmd(struct atmel_mci *host, struct mci_cmd *cmd,
			   unsigned int cmdat)
{
	unsigned flags = 0;
	unsigned cmdval = 0;

	if (host->cmd != NULL)
		dev_err(host->hw_dev, "error!\n");

	if ((atmci_readl(host, ATMCI_SR) & ATMCI_CMDRDY) == 0) {
		dev_err(host->hw_dev, "mci not ready!\n");
		return -EBUSY;
	}

	host->cmd = cmd;
	cmdval = ATMCI_CMDR_CMDNB_MASK & cmd->cmdidx;

	switch (cmd->resp_type) {
	case MMC_RSP_R1: /* short CRC, OPCODE */
	case MMC_RSP_R1b:/* short CRC, OPCODE, BUSY */
		flags |= ATMCI_CMDR_RSPTYP_48BIT;
		break;
	case MMC_RSP_R2: /* long 136 bit + CRC */
		flags |= ATMCI_CMDR_RSPTYP_136BIT;
		break;
	case MMC_RSP_R3: /* short */
		flags |= ATMCI_CMDR_RSPTYP_48BIT;
		break;
	case MMC_RSP_NONE:
		flags |= ATMCI_CMDR_RSPTYP_NONE;
		break;
	default:
		dev_err(host->hw_dev, "unhandled response type 0x%x\n",
				cmd->resp_type);
		return -EINVAL;
	}
	cmdval |= ATMCI_CMDR_RSPTYP & flags;
	cmdval |= cmdat & ~(ATMCI_CMDR_CMDNB_MASK | ATMCI_CMDR_RSPTYP);

	atmci_writel(host, ATMCI_ARGR, cmd->cmdarg);
	atmci_writel(host, ATMCI_CMDR, cmdval);

	return 0;
}

static int atmci_card_present(struct mci_host *mci)
{
	struct atmel_mci *host = to_mci_host(mci);
	int ret;

	/* No gpio, assume card is present */
	if (!gpio_is_valid(host->detect_pin))
		return 1;

	ret = gpio_get_value(host->detect_pin);

	return ret == 0 ? 1 : 0;
}

/** init the host interface */
static int atmci_reset(struct mci_host *mci, struct device_d *mci_dev)
{
	struct atmel_mci *host = to_mci_host(mci);

	clk_enable(host->clk);
	atmci_writel(host, ATMCI_DTOR, 0x7f);
	clk_disable(host->clk);

	return 0;
}

/** change host interface settings */
static void atmci_set_ios(struct mci_host *mci, struct mci_ios *ios)
{
	struct atmel_mci *host = to_mci_host(mci);

	dev_dbg(host->hw_dev, "atmel_mci_set_ios: bus_width=%d clk=%d\n",
		ios->bus_width, ios->clock);

	host->sdc_reg &= ~ATMCI_SDCBUS_MASK;
	switch (ios->bus_width) {
	case MMC_BUS_WIDTH_4:
		host->sdc_reg |= ATMCI_SDCBUS_4BIT;
		break;
	case MMC_BUS_WIDTH_8:
		host->sdc_reg |= ATMCI_SDCBUS_8BIT;
		break;
	case MMC_BUS_WIDTH_1:
		host->sdc_reg |= ATMCI_SDCBUS_1BIT;
		break;
	default:
		return;
	}

	if (ios->clock) {
		atmci_set_clk_rate(host, ios->clock);

		if (host->caps.has_cfg_reg) {
			/* setup High Speed mode in relation with card capacity */
			if (ios->timing == MMC_TIMING_SD_HS)
				host->cfg_reg |= ATMCI_CFG_HSMODE;
			else
				host->cfg_reg &= ~ATMCI_CFG_HSMODE;

			atmci_writel(host, ATMCI_CFG, host->cfg_reg);
		}
	} else {
		atmci_writel(host, ATMCI_CR, ATMCI_CR_MCIDIS);
		if (host->mode_reg) {
			atmci_readl(host, ATMCI_MR);
			clk_disable(host->clk);
		}
		host->mode_reg = 0;
	}

	return;
}

/** handle a command */
static int atmci_request(struct mci_host *mci, struct mci_cmd *cmd, struct mci_data *data)
{
	struct atmel_mci *host = to_mci_host(mci);
	u32 stat, cmdat = 0;
	int ret;

	if (host->need_reset || host->caps.need_reset_after_xfer) {
		atmci_writel(host, ATMCI_CR, ATMCI_CR_SWRST);
		atmci_writel(host, ATMCI_CR, ATMCI_CR_MCIEN);
		atmci_writel(host, ATMCI_MR, host->mode_reg);
		if (host->caps.has_cfg_reg)
			atmci_writel(host, ATMCI_CFG, host->cfg_reg);
		host->need_reset = false;
	}
	atmci_writel(host, ATMCI_SDCR, host->sdc_reg);

	if (cmd->resp_type != MMC_RSP_NONE)
		cmdat |= ATMCI_CMDR_MAXLAT_64CYC;

	if (data) {
		atmci_setup_data(host, data);

		cmdat |= ATMCI_CMDR_START_XFER | ATMCI_CMDR_MULTI_BLOCK;

		if (data->flags & MMC_DATA_READ)
			cmdat |= ATMCI_CMDR_TRDIR_READ;
	}

	ret = atmci_start_cmd(host, cmd, cmdat);
	if (ret) {
		atmci_finish_request(host);
		return ret;
	}

	stat = atmci_poll_status(host, ATMCI_CMDRDY);
	return atmci_cmd_done(host, stat);
}

static void atmci_info(struct device_d *mci_dev)
{
	struct atmel_mci *host = mci_dev->priv;

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
		gpio_is_valid(host->detect_pin) ? "yes" : "no");

}
/*
 * HSMCI (High Speed MCI) module is not fully compatible with MCI module.
 * HSMCI provides DMA support and a new config register but no more supports
 * PDC.
 */
static void atmci_get_cap(struct atmel_mci *host)
{
	unsigned int version;

	version = atmci_readl(host, ATMCI_VERSION) & 0x00000fff;
	host->version = version;

	dev_info(host->hw_dev, "version: 0x%x\n", version);

	host->caps.has_cfg_reg = 0;
	host->caps.has_highspeed = 0;
	host->caps.need_reset_after_xfer = 1;

	switch (version & 0xf00) {
	case 0x600:
	case 0x500:
		host->caps.has_odd_clk_div = 1;
	case 0x400:
	case 0x300:
		host->caps.has_cfg_reg = 1;
		host->caps.has_highspeed = 1;
	case 0x200:
		host->caps.has_rwproof = 1;
	case 0x100:
		host->caps.need_reset_after_xfer = 0;
	case 0x0:
		break;
	default:
		dev_warn(host->hw_dev,
				"Unmanaged mci version, set minimum capabilities\n");
		break;
	}
}

static int atmci_probe(struct device_d *hw_dev)
{
	struct resource *iores;
	struct atmel_mci *host;
	struct device_node *np = hw_dev->device_node;
	struct atmel_mci_platform_data *pd = hw_dev->platform_data;
	int ret;

	host = xzalloc(sizeof(*host));
	host->mci.send_cmd = atmci_request;
	host->mci.set_ios = atmci_set_ios;
	host->mci.init = atmci_reset;
	host->mci.card_present = atmci_card_present;
	host->mci.hw_dev = hw_dev;
	host->detect_pin = -EINVAL;

	if (pd) {
		host->detect_pin  = pd->detect_pin;
		host->mci.devname = pd->devname;

		if (pd->bus_width >= 4)
			host->mci.host_caps |= MMC_CAP_4_BIT_DATA;
		if (pd->bus_width == 8)
			host->mci.host_caps |= MMC_CAP_8_BIT_DATA;

		host->slot_b = pd->slot_b;
	} else if (np) {
		u32 slot_id;
		struct device_node *cnp;
		const char *alias = of_alias_get(np);

		if (alias)
			host->mci.devname = xstrdup(alias);

		host->detect_pin = of_get_named_gpio(np, "cd-gpios", 0);

		for_each_child_of_node(np, cnp) {
			if (of_property_read_u32(cnp, "reg", &slot_id)) {
				dev_warn(hw_dev, "reg property is missing for %s\n",
					 cnp->full_name);
				continue;
			}

			host->slot_b = slot_id;
			mci_of_parse_node(&host->mci, cnp);
			break;
		}
	} else {
		dev_err(hw_dev, "Missing device information\n");
		ret = -EINVAL;
		goto error_out;
	}

	if (gpio_is_valid(host->detect_pin)) {
		ret = gpio_request(host->detect_pin, "mci_cd");
		if (ret) {
			dev_err(hw_dev, "Impossible to request CD gpio %d (%d)\n",
				ret, host->detect_pin);
			goto error_out;
		}

		ret = gpio_direction_input(host->detect_pin);
		if (ret) {
			dev_err(hw_dev, "Impossible to configure CD gpio %d as input (%d)\n",
				ret, host->detect_pin);
			goto error_out;
		}
	}

	iores = dev_request_mem_resource(hw_dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	host->regs = IOMEM(iores->start);
	host->hw_dev = hw_dev;
	hw_dev->priv = host;
	host->clk = clk_get(hw_dev, "mci_clk");
	if (IS_ERR(host->clk)) {
		dev_err(hw_dev, "no mci_clk\n");
		ret = PTR_ERR(host->clk);
		goto error_out;
	}

	clk_enable(host->clk);
	atmci_writel(host, ATMCI_CR, ATMCI_CR_SWRST);
	atmci_writel(host, ATMCI_IDR, ~0UL);
	host->bus_hz = clk_get_rate(host->clk);
	clk_disable(host->clk);

	host->mci.voltages = MMC_VDD_32_33 | MMC_VDD_33_34;

	host->mci.f_min = DIV_ROUND_UP(host->bus_hz, 512);
	host->mci.f_max = host->bus_hz >> 1;

	atmci_get_cap(host);

	if (host->caps.has_highspeed)
		host->mci.host_caps |= MMC_CAP_SD_HIGHSPEED;

	if (host->slot_b)
		host->sdc_reg = ATMCI_SDCSEL_SLOT_B;
	else
		host->sdc_reg = ATMCI_SDCSEL_SLOT_A;

	if (IS_ENABLED(CONFIG_MCI_INFO))
		hw_dev->info = atmci_info;

	mci_register(&host->mci);

	return 0;

error_out:
	free(host);

	if (gpio_is_valid(host->detect_pin))
		gpio_free(host->detect_pin);

	return ret;
}

static __maybe_unused struct of_device_id atmci_compatible[] = {
	{
		.compatible = "atmel,hsmci",
	}, {
		/* sentinel */
	}
};

static struct driver_d atmci_driver = {
	.name	= "atmel_mci",
	.probe	= atmci_probe,
	.of_compatible = DRV_OF_COMPAT(atmci_compatible),
};
device_platform_driver(atmci_driver);
