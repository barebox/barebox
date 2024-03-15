// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2011 Hubert Feurstein <h.feurstein@gmail.com>
// SPDX-FileCopyrightText: 2009 Ilya Yanok <yanok@emcraft.com>
// SPDX-FileCopyrightText: 2008 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
// SPDX-FileCopyrightText: 2006 Pavel Pisa <ppisa@pikron.com>, PiKRON

#include <common.h>
#include <init.h>
#include <linux/clk.h>
#include <linux/iopoll.h>
#include <mci.h>

#include "atmel-mci-regs.h"

#ifdef __PBL__
#define udelay early_udelay
#undef  dev_err
#define dev_err(d, ...)		pr_err(__VA_ARGS__)
#undef  dev_warn
#define dev_warn(d, ...)	pr_warn(__VA_ARGS__)
#undef  dev_dbg
#define dev_dbg(d, ...)		pr_debug(__VA_ARGS__)
#undef  dev_info
#define dev_info(d, ...)	pr_info(__VA_ARGS__)
#undef	clk_enable
#define clk_enable(...)
#undef	clk_disable
#define clk_disable(...)
#endif

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

		/*
		 * Older Atmels without CLKODD have the block length
		 * in the upper 16 bits of both MCI_MR and MCI_BLKR
		 *
		 * To avoid intermittent zeroing of the block length,
		 * just hardcode 512 here and have atmci_setup_data()
		 * change it as necessary.
		 */

		host->mode_reg = ATMCI_MR_CLKDIV(clkdiv) | ATMCI_BLKLEN(512);
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
	int ret;

	ret = read_poll_timeout(atmci_readl, stat, (stat & mask), USEC_PER_SEC,
				host, ATMCI_SR);
	if (ret < 0) {
		dev_err(host->hw_dev, "timeout\n");
		host->need_reset = true;
		return ATMCI_RTOE | stat;
	}

	if (stat & STATUS_ERROR_MASK)
		return stat;

	return 0;
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

	dev_vdbg(host->hw_dev, "atmel_setup_data: nob=%d blksz=%d\n",
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
		dev_dbg(host->hw_dev, "unhandled response type 0x%x\n",
				cmd->resp_type);
		return -EINVAL;
	}
	cmdval |= ATMCI_CMDR_RSPTYP & flags;
	cmdval |= cmdat & ~(ATMCI_CMDR_CMDNB_MASK | ATMCI_CMDR_RSPTYP);

	atmci_writel(host, ATMCI_ARGR, cmd->cmdarg);
	atmci_writel(host, ATMCI_CMDR, cmdval);

	return 0;
}

/** init the host interface */
int atmci_reset(struct mci_host *mci, struct device *mci_dev)
{
	struct atmel_mci *host = to_mci_host(mci);

	clk_enable(host->clk);
	atmci_writel(host, ATMCI_DTOR, 0x7f);
	clk_disable(host->clk);

	return 0;
}

/** change host interface settings */
void atmci_common_set_ios(struct atmel_mci *host, struct mci_ios *ios)
{
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
int atmci_common_request(struct atmel_mci *host, struct mci_cmd *cmd,
			 struct mci_data *data)
{
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


/*
 * HSMCI (High Speed MCI) module is not fully compatible with MCI module.
 * HSMCI provides DMA support and a new config register but no more supports
 * PDC.
 */
void atmci_get_cap(struct atmel_mci *host)
{
	unsigned int version;

	version = atmci_readl(host, ATMCI_VERSION) & 0x00000fff;
	host->version = version;

	dev_dbg(host->hw_dev, "version: 0x%x\n", version);

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
