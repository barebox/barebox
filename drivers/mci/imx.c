/*
 *  This is a driver for the SDHC controller found in Freescale MX2/MX3
 *  SoCs. It is basically the same hardware as found on MX1 (imxmmc.c).
 *  Unlike the hardware found on MX1, this hardware just works and does
 *  not need all the quirks found in imxmmc.c, hence the separate driver.
 *
 *  Copyright (C) 2009 Ilya Yanok, <yanok@emcraft.com>
 *  Copyright (C) 2008 Sascha Hauer, Pengutronix <s.hauer@pengutronix.de>
 *  Copyright (C) 2006 Pavel Pisa, PiKRON <ppisa@pikron.com>
 *
 *  derived from pxamci.c by Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <config.h>
#include <common.h>
#include <command.h>
#include <mci.h>
#include <malloc.h>
#include <errno.h>
#include <clock.h>
#include <init.h>
#include <driver.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <io.h>

#define DRIVER_NAME "imx-mmc"

struct mxcmci_regs {
	u32 str_stp_clk;
	u32 status;
	u32 clk_rate;
	u32 cmd_dat_cont;
	u32 res_to;
	u32 read_to;
	u32 blk_len;
	u32 nob;
	u32 rev_no;
	u32 int_cntr;
	u32 cmd;
	u32 arg;
	u32 pad;
	u32 res_fifo;
	u32 buffer_access;
};

#define STR_STP_CLK_RESET               (1 << 3)
#define STR_STP_CLK_START_CLK           (1 << 1)
#define STR_STP_CLK_STOP_CLK            (1 << 0)

#define STATUS_CARD_INSERTION		(1 << 31)
#define STATUS_CARD_REMOVAL		(1 << 30)
#define STATUS_YBUF_EMPTY		(1 << 29)
#define STATUS_XBUF_EMPTY		(1 << 28)
#define STATUS_YBUF_FULL		(1 << 27)
#define STATUS_XBUF_FULL		(1 << 26)
#define STATUS_BUF_UND_RUN		(1 << 25)
#define STATUS_BUF_OVFL			(1 << 24)
#define STATUS_SDIO_INT_ACTIVE		(1 << 14)
#define STATUS_END_CMD_RESP		(1 << 13)
#define STATUS_WRITE_OP_DONE		(1 << 12)
#define STATUS_DATA_TRANS_DONE		(1 << 11)
#define STATUS_READ_OP_DONE		(1 << 11)
#define STATUS_WR_CRC_ERROR_CODE_MASK	(3 << 10)
#define STATUS_CARD_BUS_CLK_RUN		(1 << 8)
#define STATUS_BUF_READ_RDY		(1 << 7)
#define STATUS_BUF_WRITE_RDY		(1 << 6)
#define STATUS_RESP_CRC_ERR		(1 << 5)
#define STATUS_CRC_READ_ERR		(1 << 3)
#define STATUS_CRC_WRITE_ERR		(1 << 2)
#define STATUS_TIME_OUT_RESP		(1 << 1)
#define STATUS_TIME_OUT_READ		(1 << 0)
#define STATUS_ERR_MASK			0x2f

#define CMD_DAT_CONT_CMD_RESP_LONG_OFF	(1 << 12)
#define CMD_DAT_CONT_STOP_READWAIT	(1 << 11)
#define CMD_DAT_CONT_START_READWAIT	(1 << 10)
#define CMD_DAT_CONT_BUS_WIDTH_4	(2 << 8)
#define CMD_DAT_CONT_INIT		(1 << 7)
#define CMD_DAT_CONT_WRITE		(1 << 4)
#define CMD_DAT_CONT_DATA_ENABLE	(1 << 3)
#define CMD_DAT_CONT_RESPONSE_48BIT_CRC	(1 << 0)
#define CMD_DAT_CONT_RESPONSE_136BIT	(2 << 0)
#define CMD_DAT_CONT_RESPONSE_48BIT	(3 << 0)

#define INT_SDIO_INT_WKP_EN		(1 << 18)
#define INT_CARD_INSERTION_WKP_EN	(1 << 17)
#define INT_CARD_REMOVAL_WKP_EN		(1 << 16)
#define INT_CARD_INSERTION_EN		(1 << 15)
#define INT_CARD_REMOVAL_EN		(1 << 14)
#define INT_SDIO_IRQ_EN			(1 << 13)
#define INT_DAT0_EN			(1 << 12)
#define INT_BUF_READ_EN			(1 << 4)
#define INT_BUF_WRITE_EN		(1 << 3)
#define INT_END_CMD_RES_EN		(1 << 2)
#define INT_WRITE_OP_DONE_EN		(1 << 1)
#define INT_READ_OP_EN			(1 << 0)

struct mxcmci_host {
	struct mci_host		mci;
	struct mxcmci_regs	*base;
	struct clk		*clk;
	int			irq;
	int			detect_irq;
	int			dma;
	int			do_dma;
	unsigned int		power_mode;

	struct mci_cmd		*cmd;
	struct mci_data		*data;

	unsigned int		dma_nents;
	unsigned int		datasize;
	unsigned int		dma_dir;

	u16			rev_no;
	unsigned int		cmdat;

	int			clock;
};

#define to_mxcmci(mci)	container_of(mci, struct mxcmci_host, mci)

static void mxcmci_softreset(struct mxcmci_host *host)
{
	int i;

	/* reset sequence */
	writew(STR_STP_CLK_RESET, &host->base->str_stp_clk);
	writew(STR_STP_CLK_RESET | STR_STP_CLK_START_CLK,
			&host->base->str_stp_clk);

	for (i = 0; i < 8; i++)
		writew(STR_STP_CLK_START_CLK, &host->base->str_stp_clk);

	writew(0xff, &host->base->res_to);
}

static void mxcmci_setup_data(struct mxcmci_host *host, struct mci_data *data)
{
	unsigned int nob = data->blocks;
	unsigned int blksz = data->blocksize;
	unsigned int datasize = nob * blksz;

	host->data = data;

	writew(nob, &host->base->nob);
	writew(blksz, &host->base->blk_len);
	host->datasize = datasize;
}

static int mxcmci_start_cmd(struct mxcmci_host *host, struct mci_cmd *cmd,
		unsigned int cmdat)
{
	if (host->cmd != NULL)
		printf("mxcmci: error!\n");
	host->cmd = cmd;

	switch (cmd->resp_type) {
	case MMC_RSP_R1: /* short CRC, OPCODE */
	case MMC_RSP_R1b:/* short CRC, OPCODE, BUSY */
		cmdat |= CMD_DAT_CONT_RESPONSE_48BIT_CRC;
		break;
	case MMC_RSP_R2: /* long 136 bit + CRC */
		cmdat |= CMD_DAT_CONT_RESPONSE_136BIT;
		break;
	case MMC_RSP_R3: /* short */
		cmdat |= CMD_DAT_CONT_RESPONSE_48BIT;
		break;
	case MMC_RSP_NONE:
		break;
	default:
		printf("mxcmci: unhandled response type 0x%x\n",
				cmd->resp_type);
		return -EINVAL;
	}

	writew(cmd->cmdidx, &host->base->cmd);
	writel(cmd->cmdarg, &host->base->arg);
	writew(cmdat, &host->base->cmd_dat_cont);

	return 0;
}

static void mxcmci_finish_request(struct mxcmci_host *host,
		struct mci_cmd *cmd, struct mci_data *data)
{
	host->cmd = NULL;
	host->data = NULL;
}

static int mxcmci_finish_data(struct mxcmci_host *host, unsigned int stat)
{
	int data_error = 0;

	if (stat & STATUS_ERR_MASK) {
		printf("request failed. status: 0x%08x\n",
				stat);
		if (stat & STATUS_CRC_READ_ERR) {
			data_error = -EILSEQ;
		} else if (stat & STATUS_CRC_WRITE_ERR) {
			u32 err_code = (stat >> 9) & 0x3;
			if (err_code == 2) /* No CRC response */
				data_error = -ETIMEDOUT;
			else
				data_error = -EILSEQ;
		} else if (stat & STATUS_TIME_OUT_READ) {
			data_error = -ETIMEDOUT;
		} else {
			data_error = -EIO;
		}
	}

	host->data = NULL;

	return data_error;
}

static int mxcmci_read_response(struct mxcmci_host *host, unsigned int stat)
{
	struct mci_cmd *cmd = host->cmd;
	int i;
	u32 a, b, c, *resp;

	if (!cmd)
		return 0;

	resp = (u32 *)cmd->response;

	if (stat & STATUS_TIME_OUT_RESP) {
		printf("CMD TIMEOUT\n");
		return -ETIMEDOUT;
	} else if (stat & STATUS_RESP_CRC_ERR && cmd->resp_type & MMC_RSP_CRC) {
		printf("cmd crc error\n");
		return -EILSEQ;
	}

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		if (cmd->resp_type & MMC_RSP_136) {
			for (i = 0; i < 4; i++) {
				a = readw(&host->base->res_fifo);
				b = readw(&host->base->res_fifo);
				resp[i] = a << 16 | b;
			}
		} else {
			a = readw(&host->base->res_fifo);
			b = readw(&host->base->res_fifo);
			c = readw(&host->base->res_fifo);
			resp[0] = a << 24 | b << 8 | c >> 8;
		}
	}
	return 0;
}

static int mxcmci_poll_status(struct mxcmci_host *host, u32 mask)
{
	u32 stat;
	uint64_t start = get_time_ns();

	do {
		stat = readl(&host->base->status);
		if (stat & STATUS_ERR_MASK)
			return stat;
		if (is_timeout(start, SECOND))
			return STATUS_TIME_OUT_READ;
		if (stat & mask)
			return 0;
	} while (1);
}

static int mxcmci_pull(struct mxcmci_host *host, void *_buf, int bytes)
{
	unsigned int stat;
	u32 *buf = _buf;

	while (bytes > 3) {
		stat = mxcmci_poll_status(host,
				STATUS_BUF_READ_RDY | STATUS_READ_OP_DONE);
		if (stat)
			return stat;
		*buf++ = readl(&host->base->buffer_access);
		bytes -= 4;
	}

	if (bytes) {
		u8 *b = (u8 *)buf;
		u32 tmp;

		stat = mxcmci_poll_status(host,
				STATUS_BUF_READ_RDY | STATUS_READ_OP_DONE);
		if (stat)
			return stat;
		tmp = readl(&host->base->buffer_access);
		memcpy(b, &tmp, bytes);
	}

	return 0;
}

static int mxcmci_push(struct mxcmci_host *host, const void *_buf, int bytes)
{
	unsigned int stat;
	const u32 *buf = _buf;

	while (bytes > 3) {
		stat = mxcmci_poll_status(host, STATUS_BUF_WRITE_RDY);
		if (stat)
			return stat;
		writel(*buf++, &host->base->buffer_access);
		bytes -= 4;
	}

	if (bytes) {
		const u8 *b = (u8 *)buf;
		u32 tmp;

		stat = mxcmci_poll_status(host, STATUS_BUF_WRITE_RDY);
		if (stat)
			return stat;

		memcpy(&tmp, b, bytes);
		writel(tmp, &host->base->buffer_access);
	}

	stat = mxcmci_poll_status(host, STATUS_BUF_WRITE_RDY);
	if (stat)
		return stat;

	return 0;
}

static int mxcmci_transfer_data(struct mxcmci_host *host)
{
	struct mci_data *data = host->data;
	int stat;
	unsigned long length;

	length = data->blocks * data->blocksize;
	host->datasize = 0;

	if (data->flags & MMC_DATA_READ) {
		stat = mxcmci_pull(host, data->dest, length);
		if (stat)
			return stat;
		host->datasize += length;
	} else {
		stat = mxcmci_push(host, (const void *)(data->src), length);
		if (stat)
			return stat;
		host->datasize += length;
		stat = mxcmci_poll_status(host, STATUS_WRITE_OP_DONE);
		if (stat)
			return stat;
	}
	return 0;
}

static int mxcmci_cmd_done(struct mxcmci_host *host, unsigned int stat)
{
	int datastat;
	int ret;

	ret = mxcmci_read_response(host, stat);

	if (ret) {
		mxcmci_finish_request(host, host->cmd, host->data);
		return ret;
	}

	if (!host->data) {
		mxcmci_finish_request(host, host->cmd, host->data);
		return 0;
	}

	datastat = mxcmci_transfer_data(host);
	ret = mxcmci_finish_data(host, datastat);
	mxcmci_finish_request(host, host->cmd, host->data);
	return ret;
}

static int mxcmci_request(struct mci_host *mci, struct mci_cmd *cmd,
		struct mci_data *data)
{
	struct mxcmci_host *host = to_mxcmci(mci);
	unsigned int cmdat = host->cmdat;
	u32 stat;
	int ret;

	host->cmdat &= ~CMD_DAT_CONT_INIT;
	if (data) {
		mxcmci_setup_data(host, data);

		cmdat |= CMD_DAT_CONT_DATA_ENABLE;

		if (data->flags & MMC_DATA_WRITE)
			cmdat |= CMD_DAT_CONT_WRITE;
	}

	if ((ret = mxcmci_start_cmd(host, cmd, cmdat))) {
		mxcmci_finish_request(host, cmd, data);
		return ret;
	}

	do {
		stat = readl(&host->base->status);
		writel(stat, &host->base->status);
	} while (!(stat & STATUS_END_CMD_RESP));

	return mxcmci_cmd_done(host, stat);
}

static void mxcmci_set_clk_rate(struct mxcmci_host *host, unsigned int clk_ios)
{
	unsigned int divider;
	int prescaler = 0;
	unsigned long clk_in = clk_get_rate(host->clk);

	while (prescaler <= 0x800) {
		for (divider = 1; divider <= 0xF; divider++) {
			int x;

			x = (clk_in / (divider + 1));

			if (prescaler)
				x /= (prescaler * 2);

			if (x <= clk_ios)
				break;
		}
		if (divider < 0x10)
			break;

		if (prescaler == 0)
			prescaler = 1;
		else
			prescaler <<= 1;
	}

	writew((prescaler << 4) | divider, &host->base->clk_rate);
}

static void mxcmci_set_ios(struct mci_host *mci, struct mci_ios *ios)
{
	struct mxcmci_host *host = to_mxcmci(mci);

	switch (ios->bus_width) {
	case MMC_BUS_WIDTH_4:
		host->cmdat |= CMD_DAT_CONT_BUS_WIDTH_4;
		break;
	case MMC_BUS_WIDTH_1:
		host->cmdat &= ~CMD_DAT_CONT_BUS_WIDTH_4;
		break;
	default:
		return;
	}

	if (ios->clock) {
		mxcmci_set_clk_rate(host, ios->clock);
		writew(STR_STP_CLK_START_CLK, &host->base->str_stp_clk);
	} else {
		writew(STR_STP_CLK_STOP_CLK, &host->base->str_stp_clk);
	}

	host->clock = ios->clock;
}

static int mxcmci_init(struct mci_host *mci, struct device_d *dev)
{
	struct mxcmci_host *host = to_mxcmci(mci);

	mxcmci_softreset(host);

	host->rev_no = readw(&host->base->rev_no);
	if (host->rev_no != 0x400) {
		printf("wrong rev.no. 0x%08x. aborting.\n",
			host->rev_no);
		return -ENODEV;
	}

	/* recommended in data sheet */
	writew(0x2db4, &host->base->read_to);

	writel(0, &host->base->int_cntr);

	return 0;
}

static int mxcmci_probe(struct device_d *dev)
{
	struct mxcmci_host *host;
	unsigned long rate;

	host = xzalloc(sizeof(*host));

	host->clk = clk_get(dev, NULL);
	if (IS_ERR(host->clk))
		return PTR_ERR(host->clk);

	host->mci.send_cmd = mxcmci_request;
	host->mci.set_ios = mxcmci_set_ios;
	host->mci.init = mxcmci_init;
	host->mci.host_caps = MMC_CAP_4_BIT_DATA;
	host->mci.hw_dev = dev;

	host->base = dev_request_mem_region(dev, 0);

	host->mci.voltages = MMC_VDD_32_33 | MMC_VDD_33_34;

	rate = clk_get_rate(host->clk);
	host->mci.f_min = rate >> 7;
	host->mci.f_max = rate >> 1;

	mci_register(&host->mci);

	return 0;
}

static struct driver_d mxcmci_driver = {
        .name  = DRIVER_NAME,
        .probe = mxcmci_probe,
};
device_platform_driver(mxcmci_driver);
