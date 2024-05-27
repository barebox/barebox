// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2011 Robert Jarzmik

/*
 * PXA MCI driver
 * Insprired by linux kernel driver
 */

#include <common.h>
#include <errno.h>
#include <io.h>
#include <gpio.h>
#include <clock.h>
#include <init.h>
#include <mci.h>
#include <linux/err.h>

#include <mach/pxa/clock.h>
#include <mach/pxa/mci_pxa2xx.h>
#include <mach/pxa/pxa-regs.h>
#include "pxamci.h"

#define DRIVER_NAME	"pxa-mmc"

#define RX_TIMEOUT (100 * MSECOND)
#define TX_TIMEOUT (250 * MSECOND)
#define CMD_TIMEOUT (100 * MSECOND)

static void mmc_clk_enable(void)
{
	CKEN |= CKEN_MMC;
}

static int pxamci_set_power(struct pxamci_host *host, int on)
{
	mci_dbg("on=%d\n", on);
	if (host->pdata && host->pdata->gpio_power > 0)
		gpio_set_value(host->pdata->gpio_power,
			       !!on ^ host->pdata->gpio_power_invert);
	else if (host->pdata && host->pdata->setpower)
		host->pdata->setpower(&host->mci, on);
	mdelay(250);
	return 0;
}

static void pxamci_start_clock(struct pxamci_host *host)
{
	mmc_writel(START_CLOCK, MMC_STRPCL);
}

static void pxamci_stop_clock(struct pxamci_host *host)
{
	uint64_t start = get_time_ns();
	unsigned stat;

	stat = mmc_readl(MMC_STAT);
	if (stat & STAT_CLK_EN)
		writel(STOP_CLOCK, host->base + MMC_STRPCL);
	while (!is_timeout(start, CMD_TIMEOUT) && stat & STAT_CLK_EN)
		stat = mmc_readl(MMC_STAT);

	if (stat & STAT_CLK_EN)
		mci_err("unable to stop clock\n");
}

static void pxamci_setup_data(struct pxamci_host *host, struct mci_data *data)
{
	static const unsigned int timeout_ns = 1000 * MSECOND; /* 1000 ms */

	mci_dbg("nbblocks=%d, blocksize=%d\n", data->blocks, data->blocksize);
	mmc_writel(data->blocks, MMC_NOB);
	mmc_writel(data->blocksize, MMC_BLKLEN);
	mmc_writel(DIV_ROUND_UP(timeout_ns, 13128), MMC_RDTO);
}

static int pxamci_read_data(struct pxamci_host *host, unsigned char *dst,
			    unsigned len)
{
	int trf_len, trf_len1, trf_len4, ret = 0;
	uint64_t start;
	u32 *dst4;

	mci_dbg("dst=%p, len=%u\n", dst, len);
	while (!ret && len > 0) {
		trf_len = min_t(int, len, MMC_FIFO_LENGTH);

		for (start = get_time_ns(), ret = -ETIMEDOUT;
		     ret && !is_timeout(start, RX_TIMEOUT);)
			if (mmc_readl(MMC_I_REG) & RXFIFO_RD_REQ)
				ret = 0;

		trf_len1 = trf_len % 4;
		trf_len4 = trf_len / 4;
		for (dst4 = (u32 *)dst; !ret && trf_len4 > 0; trf_len4--)
			*dst4++ = mmc_readl(MMC_RXFIFO);
		for (dst = (u8 *)dst4; !ret && trf_len1 > 0; trf_len1--)
			*dst++ = mmc_readb(MMC_RXFIFO);
		len -= trf_len;
	}

	if (!ret)
		for (start = get_time_ns(), ret = -ETIMEDOUT;
		     ret && !is_timeout(start, RX_TIMEOUT);)
			if (mmc_readl(MMC_STAT) & STAT_DATA_TRAN_DONE)
				ret = 0;
	mci_dbg("ret=%d, remain=%d, stat=%x, mmc_i_reg=%x\n",
		ret, len, mmc_readl(MMC_STAT), mmc_readl(MMC_I_REG));
	return ret;
}

static int pxamci_write_data(struct pxamci_host *host, const unsigned char *src,
			    unsigned len)
{
	uint64_t start;
	int trf_len, partial = 0, ret = 0;
	unsigned stat;

	mci_dbg("src=%p, len=%u\n", src, len);
	while (!ret && len > 0) {
		trf_len = min_t(int, len, MMC_FIFO_LENGTH);
		partial = trf_len < MMC_FIFO_LENGTH;

		for (start = get_time_ns(), ret = -ETIMEDOUT;
		     ret && !is_timeout(start, TX_TIMEOUT);)
			if (mmc_readl(MMC_I_REG) & TXFIFO_WR_REQ)
				ret = 0;
		for (; !ret && trf_len > 0; trf_len--, len--)
			mmc_writeb(*src++, MMC_TXFIFO);
		if (partial)
			mmc_writeb(BUF_PART_FULL, MMC_PRTBUF);
	}

	if (!ret)
		for (start = get_time_ns(), ret = -ETIMEDOUT;
		     ret && !is_timeout(start, TX_TIMEOUT);)  {
			stat = mmc_readl(MMC_STAT);
			stat &= STAT_DATA_TRAN_DONE | STAT_PRG_DONE;
			if (stat == (STAT_DATA_TRAN_DONE | STAT_PRG_DONE))
				ret = 0;
		}
	mci_dbg("ret=%d, remain=%d, stat=%x, mmc_i_reg=%x\n",
		ret, len, mmc_readl(MMC_STAT), mmc_readl(MMC_I_REG));
	return ret;
}

static int pxamci_transfer_data(struct pxamci_host *host,
				struct mci_data *data)
{
	int nbbytes = data->blocks * data->blocksize;
	int ret;
	unsigned err_mask = STAT_CRC_READ_ERROR | STAT_CRC_WRITE_ERROR |
		STAT_READ_TIME_OUT;

	if (data->flags & MMC_DATA_WRITE)
		ret = pxamci_write_data(host, data->src, nbbytes);
	else
		ret = pxamci_read_data(host, data->dest, nbbytes);

	if (!ret && (mmc_readl(MMC_STAT) & err_mask))
		ret = -EILSEQ;
	return ret;
}

#define MMC_RSP_MASK (MMC_RSP_PRESENT | MMC_RSP_136 | MMC_RSP_CRC | \
		      MMC_RSP_BUSY | MMC_RSP_OPCODE)
static void pxamci_start_cmd(struct pxamci_host *host, struct mci_cmd *cmd,
			     unsigned int cmdat)
{
	mci_dbg("cmd=(idx=%d,type=%d,clkrt=%d)\n", cmd->cmdidx, cmd->resp_type,
		host->clkrt);

	switch (cmd->resp_type & MMC_RSP_MASK) {
	/* r1, r1b, r6, r7 */
	case MMC_RSP_R1b:
		cmdat |= CMDAT_BUSY;
	case MMC_RSP_R1:
		cmdat |= CMDAT_RESP_SHORT;
		break;
	case MMC_RSP_R2:
		cmdat |= CMDAT_RESP_R2;
		break;
	case MMC_RSP_R3:
		cmdat |= CMDAT_RESP_R3;
		break;
	default:
		break;
	}

	if (cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION)
		cmdat |= CMDAT_STOP_TRAN;

	mmc_writel(cmd->cmdidx, MMC_CMD);
	mmc_writel(cmd->cmdarg >> 16, MMC_ARGH);
	mmc_writel(cmd->cmdarg & 0xffff, MMC_ARGL);
	pxamci_start_clock(host);
	mmc_writel(cmdat, MMC_CMDAT);
}

static int pxamci_cmd_response(struct pxamci_host *host, struct mci_cmd *cmd)
{
	unsigned v, stat;
	int i;

	/*
	 * Did I mention this is Sick.  We always need to
	 * discard the upper 8 bits of the first 16-bit word.
	 */
	v = mmc_readl(MMC_RES) & 0xffff;
	for (i = 0; i < 4; i++) {
		u32 w1 = mmc_readl(MMC_RES) & 0xffff;
		u32 w2 = mmc_readl(MMC_RES) & 0xffff;
		cmd->response[i] = v << 24 | w1 << 8 | w2 >> 8;
		v = w2;
	}

	stat = mmc_readl(MMC_STAT);
	if (stat & STAT_TIME_OUT_RESPONSE)
		return -ETIMEDOUT;
	if (stat & STAT_RES_CRC_ERR && cmd->resp_type & MMC_RSP_CRC) {
		/*
		 * workaround for erratum #42:
		 * Intel PXA27x Family Processor Specification Update Rev 001
		 * A bogus CRC error can appear if the msb of a 136 bit
		 * response is a one.
		 */
		if (cpu_is_pxa27x() && cmd->resp_type & MMC_RSP_136 &&
		    cmd->response[0] & 0x80000000)
			pr_debug("ignoring CRC from command %d - *risky*\n",
				 cmd->cmdidx);
		else
			return -EILSEQ;
	}

	return 0;
}

static int pxamci_mmccmd(struct pxamci_host *host, struct mci_cmd *cmd,
			 struct mci_data *data, unsigned int cmddat)
{
	int ret = 0, stat_mask;
	uint64_t start;

	pxamci_start_cmd(host, cmd, cmddat);

	stat_mask = STAT_END_CMD_RES;
	if (cmd->resp_type & MMC_RSP_BUSY)
		stat_mask |= STAT_PRG_DONE;
	for (start = get_time_ns(), ret = -ETIMEDOUT;
	     ret && !is_timeout(start, CMD_TIMEOUT);)
		if ((mmc_readl(MMC_STAT) & stat_mask) == stat_mask)
			ret = 0;

	if (!ret && data)
		ret = pxamci_transfer_data(host, data);

	if (!ret)
		ret = pxamci_cmd_response(host, cmd);
	return ret;
}

static int pxamci_request(struct mci_host *mci, struct mci_cmd *cmd,
			  struct mci_data *data)
{
	struct pxamci_host *host = to_pxamci(mci);
	unsigned int cmdat;
	int ret;

	cmdat = host->cmdat;
	host->cmdat &= ~CMDAT_INIT;

	if (data) {
		pxamci_setup_data(host, data);

		cmdat &= ~CMDAT_BUSY;
		cmdat |= CMDAT_DATAEN;
		if (data->flags & MMC_DATA_WRITE)
			cmdat |= CMDAT_WRITE;
	}

	ret = pxamci_mmccmd(host, cmd, data, cmdat);
	return ret;
}

static void pxamci_set_ios(struct mci_host *mci, struct mci_ios *ios)
{
	struct pxamci_host *host = to_pxamci(mci);
	unsigned int clk_in = pxa_get_mmcclk();
	int fact;

	mci_dbg("bus_width=%d, clock=%u\n", ios->bus_width, ios->clock);
	if (ios->clock)
		fact = min_t(int, clk_in / ios->clock, 1 << 6);
	else
		fact = 1 << 6;
	fact = max_t(int, fact, 1);

	/*
	 * We calculate clkrt here, and will write it on the next command
	 * MMC card clock = mmcclk / (2 ^ clkrt)
	 */
	/* to handle (19.5MHz, 26MHz) */
	host->clkrt = fls(fact) - 1;

	switch (ios->bus_width) {
	case MMC_BUS_WIDTH_4:
		host->cmdat |= CMDAT_SD_4DAT;
		break;
	case MMC_BUS_WIDTH_1:
		host->cmdat &= ~CMDAT_SD_4DAT;
		break;
	default:
		return;
	}

	host->cmdat |= CMDAT_INIT;

	pxamci_set_power(host, 1);
	pxamci_stop_clock(host);
	mmc_writel(host->clkrt, MMC_CLKRT);
}

static int pxamci_init(struct mci_host *mci, struct device *dev)
{
	struct pxamci_host *host = to_pxamci(mci);

	if (host->pdata && host->pdata->init)
		return host->pdata->init(mci, dev);
	return 0;
}

static const struct mci_ops pxamci_ops = {
	.init = pxamci_init,
	.send_cmd = pxamci_request,
	.set_ios = pxamci_set_ios,
};

static int pxamci_probe(struct device *dev)
{
	struct resource *iores;
	struct pxamci_host *host;
	int gpio_power = -1;

	mmc_clk_enable();
	host = xzalloc(sizeof(*host));
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	host->base = IOMEM(iores->start);

	host->mci.ops = pxamci_ops;
	host->mci.host_caps = MMC_CAP_4_BIT_DATA;
	host->mci.hw_dev = dev;
	host->mci.voltages = MMC_VDD_32_33 | MMC_VDD_33_34;

	/*
	 * Calculate minimum clock rate, rounding up.
	 */
	host->mci.f_min = pxa_get_mmcclk() >> 6;
	host->mci.f_max = pxa_get_mmcclk();

	/*
	 * Ensure that the host controller is shut down, and setup
	 * with our defaults.
	 */
	pxamci_stop_clock(host);
	mmc_writel(0, MMC_SPI);
	mmc_writel(64, MMC_RESTO);
	mmc_writel(0, MMC_I_MASK);

	host->pdata = dev->platform_data;
	if (host->pdata)
		gpio_power = host->pdata->gpio_power;

	if (gpio_power > 0)
		gpio_direction_output(gpio_power,
				      host->pdata->gpio_power_invert);

	mci_register(&host->mci);
	return 0;
}

static struct driver pxamci_driver = {
	.name  = DRIVER_NAME,
	.probe = pxamci_probe,
};
device_platform_driver(pxamci_driver);
