/*
 * Copyright (C) 2013 Altera Corporation <www.altera.com>
 *
 * (C) Copyright 2012 SAMSUNG Electronics
 * Jaehoon Chung <jh80.chung@samsung.com>
 * Rajeshawari Shinde <rajeshwari.s@samsung.com>
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
#include <dma.h>
#include <driver.h>
#include <malloc.h>
#include <clock.h>
#include <init.h>
#include <mci.h>
#include <io.h>
#include <platform_data/dw_mmc.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <asm-generic/errno.h>

#define DWMCI_CTRL		0x000
#define	DWMCI_PWREN		0x004
#define DWMCI_CLKDIV		0x008
#define DWMCI_CLKSRC		0x00C
#define DWMCI_CLKENA		0x010
#define DWMCI_TMOUT		0x014
#define DWMCI_CTYPE		0x018
#define DWMCI_BLKSIZ		0x01C
#define DWMCI_BYTCNT		0x020
#define DWMCI_INTMASK		0x024
#define DWMCI_CMDARG		0x028
#define DWMCI_CMD		0x02C
#define DWMCI_RESP0		0x030
#define DWMCI_RESP1		0x034
#define DWMCI_RESP2		0x038
#define DWMCI_RESP3		0x03C
#define DWMCI_MINTSTS		0x040
#define DWMCI_RINTSTS		0x044
#define DWMCI_STATUS		0x048
#define DWMCI_FIFOTH		0x04C
#define DWMCI_CDETECT		0x050
#define DWMCI_WRTPRT		0x054
#define DWMCI_GPIO		0x058
#define DWMCI_TCMCNT		0x05C
#define DWMCI_TBBCNT		0x060
#define DWMCI_DEBNCE		0x064
#define DWMCI_USRID		0x068
#define DWMCI_VERID		0x06C
#define DWMCI_HCON		0x070
#define DWMCI_UHS_REG		0x074
#define DWMCI_BMOD		0x080
#define DWMCI_PLDMND		0x084
#define DWMCI_DBADDR		0x088
#define DWMCI_IDSTS		0x08C
#define DWMCI_IDINTEN		0x090
#define DWMCI_DSCADDR		0x094
#define DWMCI_BUFADDR		0x098
#define DWMCI_DATA		0x200

/* Interrupt Mask register */
#define DWMCI_INTMSK_ALL		0xffffffff
#define DWMCI_INTMSK_RE			(1 << 1)
#define DWMCI_INTMSK_CDONE		(1 << 2)
#define DWMCI_INTMSK_DTO		(1 << 3)
#define DWMCI_INTMSK_TXDR		(1 << 4)
#define DWMCI_INTMSK_RXDR		(1 << 5)
#define DWMCI_INTMSK_RCRC		(1 << 6)
#define DWMCI_INTMSK_DCRC		(1 << 7)
#define DWMCI_INTMSK_RTO		(1 << 8)
#define DWMCI_INTMSK_DRTO		(1 << 9)
#define DWMCI_INTMSK_HTO		(1 << 10)
#define DWMCI_INTMSK_FRUN		(1 << 11)
#define DWMCI_INTMSK_HLE		(1 << 12)
#define DWMCI_INTMSK_SBE		(1 << 13)
#define DWMCI_INTMSK_ACD		(1 << 14)
#define DWMCI_INTMSK_EBE		(1 << 15)

/* Raw interrupt Register */
#define DWMCI_DATA_ERR	(DWMCI_INTMSK_EBE | DWMCI_INTMSK_SBE | DWMCI_INTMSK_HLE |\
			DWMCI_INTMSK_FRUN | DWMCI_INTMSK_DCRC)
#define DWMCI_DATA_TOUT	(DWMCI_INTMSK_HTO | DWMCI_INTMSK_DRTO)

/* CTRL register */
#define DWMCI_CTRL_RESET		(1 << 0)
#define DWMCI_CTRL_FIFO_RESET		(1 << 1)
#define DWMCI_CTRL_DMA_RESET		(1 << 2)
#define DWMCI_DMA_EN			(1 << 5)
#define DWMCI_CTRL_SEND_AS_CCSD		(1 << 10)
#define DWMCI_IDMAC_EN			(1 << 25)
#define DWMCI_RESET_ALL			(DWMCI_CTRL_RESET | DWMCI_CTRL_FIFO_RESET |\
				DWMCI_CTRL_DMA_RESET)

/* CMD register */
#define DWMCI_CMD_RESP_EXP		(1 << 6)
#define DWMCI_CMD_RESP_LENGTH		(1 << 7)
#define DWMCI_CMD_CHECK_CRC		(1 << 8)
#define DWMCI_CMD_DATA_EXP		(1 << 9)
#define DWMCI_CMD_RW			(1 << 10)
#define DWMCI_CMD_SEND_STOP		(1 << 12)
#define DWMCI_CMD_ABORT_STOP		(1 << 14)
#define DWMCI_CMD_PRV_DAT_WAIT		(1 << 13)
#define DWMCI_CMD_UPD_CLK		(1 << 21)
#define DWMCI_CMD_USE_HOLD_REG		(1 << 29)
#define DWMCI_CMD_START			(1 << 31)

/* CLKENA register */
#define DWMCI_CLKEN_ENABLE		(1 << 0)
#define DWMCI_CLKEN_LOW_PWR		(1 << 16)

/* Card-type register */
#define DWMCI_CTYPE_1BIT		0
#define DWMCI_CTYPE_4BIT		(1 << 0)
#define DWMCI_CTYPE_8BIT		(1 << 16)

/* Status Register */
#define DWMCI_STATUS_FIFO_EMPTY		(1 << 2)
#define DWMCI_STATUS_FIFO_FULL		(1 << 3)
#define DWMCI_STATUS_BUSY		(1 << 9)

/* FIFOTH Register */
#define DWMCI_FIFOTH_MSIZE(x)		((x) << 28)
#define DWMCI_FIFOTH_RX_WMARK(x)	((x) << 16)
#define DWMCI_FIFOTH_TX_WMARK(x)	(x)
#define DWMCI_FIFOTH_FIFO_DEPTH(x)	((((x) >> 16) & 0x3ff) + 1)

#define DWMCI_IDMAC_OWN			(1 << 31)
#define DWMCI_IDMAC_CH			(1 << 4)
#define DWMCI_IDMAC_FS			(1 << 3)
#define DWMCI_IDMAC_LD			(1 << 2)

/* Bus Mode Register */
#define DWMCI_BMOD_IDMAC_RESET		(1 << 0)
#define DWMCI_BMOD_IDMAC_FB		(1 << 1)
#define DWMCI_BMOD_IDMAC_EN		(1 << 7)

struct dwmci_host {
	struct mci_host mci;
	struct device_d *dev;
	struct clk *clk_biu, *clk_ciu;
	void *ioaddr;
	unsigned int fifo_size_bytes;

	struct dwmci_idmac *idmac;
	unsigned long clkrate;
	int ciu_div;
	u32 fifoth_val;
	u32 pwren_value;
};

struct dwmci_idmac {
	uint32_t flags;
	uint32_t cnt;
	uint32_t addr;
	uint32_t next_addr;
};

static inline int dwmci_use_pio(struct dwmci_host *host)
{
	return IS_ENABLED(CONFIG_MCI_DW_PIO);
}

static inline void dwmci_writel(struct dwmci_host *host, int reg, uint32_t val)
{
	writel(val, host->ioaddr + reg);
}

static inline uint32_t dwmci_readl(struct dwmci_host *host, int reg)
{
	return readl(host->ioaddr + reg);
}

#define DW_MMC_NUM_IDMACS	(PAGE_SIZE / sizeof(struct dwmci_idmac))

static inline struct dwmci_host *to_dwmci_host(struct mci_host *mci)
{
	return container_of(mci, struct dwmci_host, mci);
}

static int dwmci_wait_reset(struct dwmci_host *host, uint32_t value)
{
	uint64_t start;
	uint32_t ctrl;

	start = get_time_ns();

	dwmci_writel(host, DWMCI_CTRL, value);

	while (!is_timeout(start, SECOND)) {
		ctrl = dwmci_readl(host, DWMCI_CTRL);
		if (!(ctrl & DWMCI_RESET_ALL))
			return 0;
	}

	return -EIO;
}

static int dwmci_prepare_data_pio(struct dwmci_host *host,
		struct mci_data *data)
{
	unsigned long ctrl;

	dwmci_wait_reset(host, DWMCI_CTRL_FIFO_RESET);
	dwmci_writel(host, DWMCI_RINTSTS,
			DWMCI_INTMSK_TXDR | DWMCI_INTMSK_RXDR);

	ctrl = dwmci_readl(host, DWMCI_INTMASK);
	ctrl |= DWMCI_INTMSK_TXDR | DWMCI_INTMSK_RXDR;
	dwmci_writel(host, DWMCI_INTMASK, ctrl);

	ctrl = dwmci_readl(host, DWMCI_CTRL);
	ctrl &= ~(DWMCI_IDMAC_EN | DWMCI_DMA_EN);
	dwmci_writel(host, DWMCI_CTRL, ctrl);

	dwmci_writel(host, DWMCI_FIFOTH, host->fifoth_val);

	dwmci_writel(host, DWMCI_TMOUT, 0xFFFFFFFF);
	dwmci_writel(host, DWMCI_BLKSIZ, data->blocksize);
	dwmci_writel(host, DWMCI_BYTCNT, data->blocksize * data->blocks);

	return 0;
}

static int dwmci_prepare_data_dma(struct dwmci_host *host,
		struct mci_data *data)
{
	unsigned long ctrl;
	unsigned int i = 0, flags, cnt, blk_cnt;
	unsigned long data_start, start_addr;
	struct dwmci_idmac *desc = host->idmac;

	blk_cnt = data->blocks;

	if (blk_cnt > DW_MMC_NUM_IDMACS)
		return -EINVAL;

	dwmci_wait_reset(host, DWMCI_CTRL_FIFO_RESET);

	data_start = (uint32_t)desc;
	dwmci_writel(host, DWMCI_DBADDR, (uint32_t)desc);

	if (data->flags & MMC_DATA_READ)
		start_addr = (uint32_t)data->dest;
	else
		start_addr = (uint32_t)data->src;

	do {
		flags = DWMCI_IDMAC_OWN | DWMCI_IDMAC_CH;
		flags |= (i == 0) ? DWMCI_IDMAC_FS : 0;

		if (blk_cnt <= 8) {
			flags |= DWMCI_IDMAC_LD;
			cnt = data->blocksize * blk_cnt;
		} else {
			cnt = data->blocksize * 8;
		}

		desc->flags = flags;
		desc->cnt = cnt;
		desc->addr = start_addr + (i * PAGE_SIZE);
		desc->next_addr = (uint32_t)(desc + 1);

		dev_dbg(host->dev, "desc@ 0x%p 0x%08x 0x%08x 0x%08x 0x%08x\n",
				desc, flags, cnt, desc->addr, desc->next_addr);
		if (blk_cnt < 8)
			break;

		blk_cnt -= 8;
		desc++;
		i++;
	} while (1);

	ctrl = dwmci_readl(host, DWMCI_CTRL);
	ctrl |= DWMCI_IDMAC_EN | DWMCI_DMA_EN;
	dwmci_writel(host, DWMCI_CTRL, ctrl);

	ctrl = dwmci_readl(host, DWMCI_BMOD);
	ctrl |= DWMCI_BMOD_IDMAC_FB | DWMCI_BMOD_IDMAC_EN;
	dwmci_writel(host, DWMCI_BMOD, ctrl);

	dwmci_writel(host, DWMCI_BLKSIZ, data->blocksize);
	dwmci_writel(host, DWMCI_BYTCNT, data->blocksize * data->blocks);

	return 0;
}

static int dwmci_prepare_data(struct dwmci_host *host,
		struct mci_data *data)
{
	if (dwmci_use_pio(host))
		return dwmci_prepare_data_pio(host, data);
	else
		return dwmci_prepare_data_dma(host, data);
}

static int dwmci_set_transfer_mode(struct dwmci_host *host,
		struct mci_data *data)
{
	unsigned long mode;

	mode = DWMCI_CMD_DATA_EXP;
	if (data->flags & MMC_DATA_WRITE)
		mode |= DWMCI_CMD_RW;

	return mode;
}

static int dwmci_read_data_pio(struct dwmci_host *host, struct mci_data *data)
{
	u32 *pdata = (u32 *)data->dest;
	u32 val, status, timeout;
	u32 rcnt, rlen = 0;

	for (rcnt = (data->blocksize * data->blocks)>>2; rcnt; rcnt--) {
		timeout = 20000;
		status = dwmci_readl(host, DWMCI_STATUS);
		while (--timeout
		    && (status & DWMCI_STATUS_FIFO_EMPTY)) {
			udelay(200);
			status = dwmci_readl(host, DWMCI_STATUS);
		}
		if (!timeout) {
			dev_err(host->dev, "%s: FIFO underflow timeout\n",
			    __func__);
			break;
		}

		val = dwmci_readl(host, DWMCI_DATA);
		*pdata++ = val;
		rlen += 4;
	}
	dwmci_writel(host, DWMCI_RINTSTS, DWMCI_INTMSK_RXDR);

	return rlen;
}

static int dwmci_write_data_pio(struct dwmci_host *host, struct mci_data *data)
{
	u32 *pdata = (u32 *)data->src;
	u32 status, timeout;
	u32 wcnt, wlen = 0;

	for (wcnt = (data->blocksize * data->blocks)>>2; wcnt; wcnt--) {
		timeout = 20000;
		status = dwmci_readl(host, DWMCI_STATUS);
		while (--timeout
		    && (status & DWMCI_STATUS_FIFO_FULL)) {
			udelay(200);
			status = dwmci_readl(host, DWMCI_STATUS);
		}
		if (!timeout) {
			dev_err(host->dev, "%s: FIFO overflow timeout\n",
			    __func__);
			break;
		}
		dwmci_writel(host, DWMCI_DATA, *pdata++);
		wlen += 4;
	}
	dwmci_writel(host, DWMCI_RINTSTS, DWMCI_INTMSK_TXDR);

	/* Wait for FIFO is flushed for slow-speed cards */
	timeout = 20000;
	status = dwmci_readl(host, DWMCI_STATUS);
	while (--timeout
	    && !(status & DWMCI_STATUS_FIFO_EMPTY)) {
		udelay(10);
		status = dwmci_readl(host, DWMCI_STATUS);
	}
	if (!timeout) {
		dev_err(host->dev, "%s: FIFO flush timeout\n",
		    __func__);
		return -EIO;
	}

	return wlen;
}

static int
dwmci_cmd(struct mci_host *mci, struct mci_cmd *cmd, struct mci_data *data)
{
	struct dwmci_host *host = to_dwmci_host(mci);
	int flags = 0;
	uint32_t mask;
	uint32_t ctrl;
	uint64_t start;
	int ret;
	unsigned int num_bytes = 0;

	start = get_time_ns();
	while (1) {
		if (!(dwmci_readl(host, DWMCI_STATUS) & DWMCI_STATUS_BUSY))
			break;

		if (is_timeout(start, 100 * MSECOND)) {
			dev_dbg(host->dev, "Timeout on data busy\n");
			return -ETIMEDOUT;
		}
	}

	dwmci_writel(host, DWMCI_RINTSTS, DWMCI_INTMSK_ALL);

	if (data) {
		num_bytes = data->blocks * data->blocksize;

		if (data->flags & MMC_DATA_WRITE)
			dma_sync_single_for_device((unsigned long)data->src,
						   num_bytes, DMA_TO_DEVICE);
		else
			dma_sync_single_for_device((unsigned long)data->dest,
						   num_bytes, DMA_FROM_DEVICE);

		ret = dwmci_prepare_data(host, data);
		if (ret)
			return ret;
	}

	dwmci_writel(host, DWMCI_CMDARG, cmd->cmdarg);

	if (data)
		flags = dwmci_set_transfer_mode(host, data);

	if ((cmd->resp_type & MMC_RSP_136) && (cmd->resp_type & MMC_RSP_BUSY))
		return -EINVAL;

	if (cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION)
		flags |= DWMCI_CMD_ABORT_STOP;
	else
		flags |= DWMCI_CMD_PRV_DAT_WAIT;

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		flags |= DWMCI_CMD_RESP_EXP;
		if (cmd->resp_type & MMC_RSP_136)
			flags |= DWMCI_CMD_RESP_LENGTH;
	}

	if (cmd->resp_type & MMC_RSP_CRC)
		flags |= DWMCI_CMD_CHECK_CRC;

	flags |= (cmd->cmdidx | DWMCI_CMD_START | DWMCI_CMD_USE_HOLD_REG);

	dev_dbg(host->dev, "Sending CMD%d\n", cmd->cmdidx);

	dwmci_writel(host, DWMCI_CMD, flags);

	start = get_time_ns();
	while (1) {
		mask = dwmci_readl(host, DWMCI_RINTSTS);
		if (mask & DWMCI_INTMSK_CDONE) {
			if (!data)
				dwmci_writel(host, DWMCI_RINTSTS, mask);
			break;
		}
		if (is_timeout(start, 100 * MSECOND)) {
			dev_dbg(host->dev, "Send command timeout..\n");
			return -ETIMEDOUT;
		}
	}

	if (mask & DWMCI_INTMSK_RTO) {
		dev_dbg(host->dev, "Response Timeout..\n");
		return -ETIMEDOUT;
	} else if (mask & DWMCI_INTMSK_RE) {
		dev_dbg(host->dev, "Response Error..\n");
		return -EIO;
	}

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		if (cmd->resp_type & MMC_RSP_136) {
			cmd->response[0] = dwmci_readl(host, DWMCI_RESP3);
			cmd->response[1] = dwmci_readl(host, DWMCI_RESP2);
			cmd->response[2] = dwmci_readl(host, DWMCI_RESP1);
			cmd->response[3] = dwmci_readl(host, DWMCI_RESP0);
		} else {
			cmd->response[0] = dwmci_readl(host, DWMCI_RESP0);
		}
	}

	if (data) {
		start = get_time_ns();
		do {
			mask = dwmci_readl(host, DWMCI_RINTSTS);

			if (mask & (DWMCI_DATA_ERR)) {
				dev_dbg(host->dev, "DATA ERROR!\n");
				return -EIO;
			}

			if (!dwmci_use_pio(host) && (mask & DWMCI_DATA_TOUT)) {
				dev_dbg(host->dev, "DATA TIMEOUT!\n");
				return -EIO;
			}

			if (is_timeout(start, SECOND * 10)) {
				dev_dbg(host->dev, "Data timeout\n");
				return -ETIMEDOUT;
			}

			if (dwmci_use_pio(host) && (mask & DWMCI_INTMSK_RXDR)) {
				dwmci_read_data_pio(host, data);
				mask = dwmci_readl(host, DWMCI_RINTSTS);
			}
			if (dwmci_use_pio(host) && (mask & DWMCI_INTMSK_TXDR)) {
				dwmci_write_data_pio(host, data);
				mask = dwmci_readl(host, DWMCI_RINTSTS);
			}
		} while (!(mask & DWMCI_INTMSK_DTO));

		dwmci_writel(host, DWMCI_RINTSTS, mask);

		if (!dwmci_use_pio(host)) {
			ctrl = dwmci_readl(host, DWMCI_CTRL);
			ctrl &= ~(DWMCI_DMA_EN);
			dwmci_writel(host, DWMCI_CTRL, ctrl);

			if (data->flags & MMC_DATA_WRITE)
				dma_sync_single_for_cpu((unsigned long)data->src,
							num_bytes, DMA_TO_DEVICE);
			else
				dma_sync_single_for_cpu((unsigned long)data->dest,
							num_bytes, DMA_FROM_DEVICE);
		}
	}

	udelay(100);

	return 0;
}

static int dwmci_send_cmd(struct dwmci_host *host, u32 cmd, u32 arg)
{
	uint64_t start = get_time_ns();
	uint32_t status;

	dwmci_writel(host, DWMCI_CMDARG, arg);

	dwmci_writel(host, DWMCI_CMD, cmd | DWMCI_CMD_START);

	while (1) {
		status = dwmci_readl(host, DWMCI_CMD);
		if (!(status & DWMCI_CMD_START))
			return 0;

		if (is_timeout(start, 100 * MSECOND)) {
			dev_err(host->dev, "TIMEOUT error!!\n");
			return -ETIMEDOUT;
		}
	}
}

static int dwmci_setup_bus(struct dwmci_host *host, uint32_t freq)
{
	uint32_t div;
	unsigned long sclk;

	sclk = host->clkrate / host->ciu_div;

	div = DIV_ROUND_UP(sclk, freq);
	if (div > 510)
		div = 510;

	div >>= 1;

	dwmci_writel(host, DWMCI_CLKENA, 0);
	dwmci_writel(host, DWMCI_CLKSRC, 0);

	dwmci_writel(host, DWMCI_CLKDIV, div);

	dwmci_send_cmd(host, DWMCI_CMD_PRV_DAT_WAIT | DWMCI_CMD_UPD_CLK, 0);

	dwmci_writel(host, DWMCI_CLKENA, DWMCI_CLKEN_ENABLE);

	dwmci_send_cmd(host, DWMCI_CMD_PRV_DAT_WAIT | DWMCI_CMD_UPD_CLK, 0);

	return 0;
}

static void dwmci_set_ios(struct mci_host *mci, struct mci_ios *ios)
{
	struct dwmci_host *host = to_dwmci_host(mci);
	uint32_t ctype;

	dev_dbg(host->dev, "Buswidth = %d, clock: %d\n", ios->bus_width, ios->clock);

	if (ios->clock)
		dwmci_setup_bus(host, ios->clock);

	switch (ios->bus_width) {
	case MMC_BUS_WIDTH_8:
		ctype = DWMCI_CTYPE_8BIT;
		break;
	case MMC_BUS_WIDTH_4:
		ctype = DWMCI_CTYPE_4BIT;
		break;
	default:
		ctype = DWMCI_CTYPE_1BIT;
		break;
	}

	dwmci_writel(host, DWMCI_CTYPE, ctype);
}

static int dwmci_card_present(struct mci_host *mci)
{
	return 1;
}

static int dwmci_init(struct mci_host *mci, struct device_d *dev)
{
	struct dwmci_host *host = to_dwmci_host(mci);
	uint32_t fifo_size;

	dwmci_writel(host, DWMCI_PWREN, host->pwren_value);

	if (dwmci_wait_reset(host, DWMCI_RESET_ALL)) {
		dev_err(host->dev, "reset failed\n");
		return -EIO;
	}

	dwmci_writel(host, DWMCI_RINTSTS, 0xffffffff);
	dwmci_writel(host, DWMCI_INTMASK, 0);

	dwmci_writel(host, DWMCI_TMOUT, 0xffffffff);

	dwmci_writel(host, DWMCI_IDINTEN, 0);
	dwmci_writel(host, DWMCI_BMOD, 1);

	fifo_size = dwmci_readl(host, DWMCI_FIFOTH);

	/*
	 * Use reset default of the rx_wmark field to determine the
	 * fifo depth.
	 */
	fifo_size = DWMCI_FIFOTH_FIFO_DEPTH(fifo_size);
	host->fifo_size_bytes = fifo_size * 4;

	/*
	 * If fifo-depth property is set, use this value
	 */
	if (!of_property_read_u32(host->dev->device_node,
		    "fifo-depth", &fifo_size)) {
		host->fifo_size_bytes = fifo_size;
		dev_dbg(host->dev, "Using fifo-depth=%u\n",
		    host->fifo_size_bytes);
	}

	host->fifoth_val = DWMCI_FIFOTH_MSIZE(0x2) |
		DWMCI_FIFOTH_RX_WMARK(fifo_size / 2 - 1) |
		DWMCI_FIFOTH_TX_WMARK(fifo_size / 2);

	dwmci_writel(host, DWMCI_FIFOTH, host->fifoth_val);

	dwmci_writel(host, DWMCI_CLKENA, 0);
	dwmci_writel(host, DWMCI_CLKSRC, 0);

	return 0;
}

static int dw_mmc_detect(struct device_d *dev)
{
	struct dwmci_host *host = dev->priv;

	return mci_detect_card(&host->mci);
}

static int dw_mmc_probe(struct device_d *dev)
{
	struct resource *iores;
	struct dwmci_host *host;
	struct dw_mmc_platform_data *pdata = dev->platform_data;

	host = xzalloc(sizeof(*host));

	host->clk_biu = clk_get(dev, "biu");
	if (IS_ERR(host->clk_biu))
		return PTR_ERR(host->clk_biu);

	host->clk_ciu = clk_get(dev, "ciu");
	if (IS_ERR(host->clk_ciu))
		return PTR_ERR(host->clk_ciu);

	clk_enable(host->clk_biu);
	clk_enable(host->clk_ciu);

	host->dev = dev;
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	host->ioaddr = IOMEM(iores->start);

	host->idmac = dma_alloc_coherent(sizeof(*host->idmac) * DW_MMC_NUM_IDMACS,
					 DMA_ADDRESS_BROKEN);

	host->mci.send_cmd = dwmci_cmd;
	host->mci.set_ios = dwmci_set_ios;
	host->mci.init = dwmci_init;
	host->mci.card_present = dwmci_card_present;
	host->mci.hw_dev = dev;
	host->mci.voltages = MMC_VDD_32_33 | MMC_VDD_33_34;
	host->mci.host_caps = MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA;
	host->mci.host_caps |= MMC_CAP_MMC_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED_52MHZ |
			       MMC_CAP_SD_HIGHSPEED;

	if (pdata) {
		host->ciu_div = pdata->ciu_div;
		host->mci.host_caps &= ~MMC_CAP_BIT_DATA_MASK;
		host->mci.host_caps |= pdata->bus_width_caps;
	} else if (dev->device_node) {
		const char *alias = of_alias_get(dev->device_node);
		if (alias)
			host->mci.devname = xstrdup(alias);
		of_property_read_u32(dev->device_node, "dw-mshc-ciu-div",
				&host->ciu_div);
	}

	/* divider is 0 based in pdata and 1 based in our private struct */
	host->ciu_div++;

	if (of_device_is_compatible(dev->device_node,
	    "rockchip,rk2928-dw-mshc"))
		host->pwren_value = 0;
	else
		host->pwren_value = 1;

	dev->detect = dw_mmc_detect;

	host->clkrate = clk_get_rate(host->clk_ciu);
	host->mci.f_min = host->clkrate / 510 / host->ciu_div;
	if (host->mci.f_min < 200000)
		host->mci.f_min = 200000;
	host->mci.f_max = host->clkrate / host->ciu_div;

	mci_of_parse(&host->mci);

	dev->priv = host;

	return mci_register(&host->mci);
}

static __maybe_unused struct of_device_id dw_mmc_compatible[] = {
	{
		.compatible = "altr,socfpga-dw-mshc",
	}, {
		.compatible = "rockchip,rk2928-dw-mshc",
	}, {
		.compatible = "rockchip,rk3288-dw-mshc",
	}, {
		/* sentinel */
	}
};

static struct driver_d dw_mmc_driver = {
	.name  = "dw_mmc",
	.probe = dw_mmc_probe,
	.of_compatible = DRV_OF_COMPAT(dw_mmc_compatible),
};
device_platform_driver(dw_mmc_driver);
