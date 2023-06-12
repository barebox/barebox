// SPDX-License-Identifier: GPL-2.0-only
/*
 * This code is ported from U-Boot by Lucas Stach <l.stach@pengutronix.de> and
 * has the following contributors listed in the original license header:
 * Alexander Graf <agraf@suse.de>
 * Phil Elwell <phil@raspberrypi.org>
 * Gellert Weisz
 * Stephen Warren
 * Oleksandr Tymoshenko
 */

#include <clock.h>
#include <common.h>
#include <driver.h>
#include <init.h>
#include <linux/clk.h>
#include <linux/iopoll.h>
#include <linux/log2.h>
#include <mci.h>

#define SDCMD  0x00 /* Command to SD card              - 16 R/W */
#define SDARG  0x04 /* Argument to SD card             - 32 R/W */
#define SDTOUT 0x08 /* Start value for timeout counter - 32 R/W */
#define SDCDIV 0x0c /* Start value for clock divider   - 11 R/W */
#define SDRSP0 0x10 /* SD card response (31:0)         - 32 R   */
#define SDRSP1 0x14 /* SD card response (63:32)        - 32 R   */
#define SDRSP2 0x18 /* SD card response (95:64)        - 32 R   */
#define SDRSP3 0x1c /* SD card response (127:96)       - 32 R   */
#define SDHSTS 0x20 /* SD host status                  - 11 R/W */
#define SDVDD  0x30 /* SD card power control           -  1 R/W */
#define SDEDM  0x34 /* Emergency Debug Mode            - 13 R/W */
#define SDHCFG 0x38 /* Host configuration              -  2 R/W */
#define SDHBCT 0x3c /* Host byte count (debug)         - 32 R/W */
#define SDDATA 0x40 /* Data to/from SD card            - 32 R/W */
#define SDHBLC 0x50 /* Host block count (SDIO/SDHC)    -  9 R/W */

#define SDCMD_NEW_FLAG			0x8000
#define SDCMD_FAIL_FLAG			0x4000
#define SDCMD_BUSYWAIT			0x800
#define SDCMD_NO_RESPONSE		0x400
#define SDCMD_LONG_RESPONSE		0x200
#define SDCMD_WRITE_CMD			0x80
#define SDCMD_READ_CMD			0x40
#define SDCMD_CMD_MASK			0x3f

#define SDCDIV_MAX_CDIV			0x7ff

#define SDHSTS_BUSY_IRPT		0x400
#define SDHSTS_BLOCK_IRPT		0x200
#define SDHSTS_SDIO_IRPT		0x100
#define SDHSTS_REW_TIME_OUT		0x80
#define SDHSTS_CMD_TIME_OUT		0x40
#define SDHSTS_CRC16_ERROR		0x20
#define SDHSTS_CRC7_ERROR		0x10
#define SDHSTS_FIFO_ERROR		0x08
#define SDHSTS_DATA_FLAG		0x01

#define SDHSTS_CLEAR_MASK		(SDHSTS_BUSY_IRPT | \
					 SDHSTS_BLOCK_IRPT | \
					 SDHSTS_SDIO_IRPT | \
					 SDHSTS_REW_TIME_OUT | \
					 SDHSTS_CMD_TIME_OUT | \
					 SDHSTS_CRC16_ERROR | \
					 SDHSTS_CRC7_ERROR | \
					 SDHSTS_FIFO_ERROR)

#define SDHSTS_TRANSFER_ERROR_MASK	(SDHSTS_CRC7_ERROR | \
					 SDHSTS_CRC16_ERROR | \
					 SDHSTS_REW_TIME_OUT | \
					 SDHSTS_FIFO_ERROR)

#define SDHSTS_ERROR_MASK		(SDHSTS_CMD_TIME_OUT | \
					 SDHSTS_TRANSFER_ERROR_MASK)

#define SDHCFG_BUSY_IRPT_EN	BIT(10)
#define SDHCFG_BLOCK_IRPT_EN	BIT(8)
#define SDHCFG_SDIO_IRPT_EN	BIT(5)
#define SDHCFG_DATA_IRPT_EN	BIT(4)
#define SDHCFG_SLOW_CARD	BIT(3)
#define SDHCFG_WIDE_EXT_BUS	BIT(2)
#define SDHCFG_WIDE_INT_BUS	BIT(1)
#define SDHCFG_REL_CMD_LINE	BIT(0)

#define SDVDD_POWER_OFF		0
#define SDVDD_POWER_ON		1

#define SDEDM_FORCE_DATA_MODE	BIT(19)
#define SDEDM_CLOCK_PULSE	BIT(20)
#define SDEDM_BYPASS		BIT(21)

#define SDEDM_FIFO_FILL_SHIFT	4
#define SDEDM_FIFO_FILL_MASK	0x1f
static u32 edm_fifo_fill(u32 edm)
{
	return (edm >> SDEDM_FIFO_FILL_SHIFT) & SDEDM_FIFO_FILL_MASK;
}

#define SDEDM_WRITE_THRESHOLD_SHIFT	9
#define SDEDM_READ_THRESHOLD_SHIFT	14
#define SDEDM_THRESHOLD_MASK		0x1f

#define SDEDM_FSM_MASK		0xf
#define SDEDM_FSM_IDENTMODE	0x0
#define SDEDM_FSM_DATAMODE	0x1
#define SDEDM_FSM_READDATA	0x2
#define SDEDM_FSM_WRITEDATA	0x3
#define SDEDM_FSM_READWAIT	0x4
#define SDEDM_FSM_READCRC	0x5
#define SDEDM_FSM_WRITECRC	0x6
#define SDEDM_FSM_WRITEWAIT1	0x7
#define SDEDM_FSM_POWERDOWN	0x8
#define SDEDM_FSM_POWERUP	0x9
#define SDEDM_FSM_WRITESTART1	0xa
#define SDEDM_FSM_WRITESTART2	0xb
#define SDEDM_FSM_GENPULSES	0xc
#define SDEDM_FSM_WRITEWAIT2	0xd
#define SDEDM_FSM_STARTPOWDOWN	0xf

#define SDDATA_FIFO_WORDS	16

#define FIFO_READ_THRESHOLD	4
#define FIFO_WRITE_THRESHOLD	4
#define SDDATA_FIFO_PIO_BURST	8

#define SDHST_TIMEOUT_MAX_USEC	100000

struct bcm2835_host {
	struct mci_host		mci;
	void __iomem		*regs;
	struct clk		*clk;
};

static inline struct bcm2835_host *to_bcm2835_host(struct mci_host *mci)
{
	return container_of(mci, struct bcm2835_host, mci);
}

static int bcm2835_sdhost_init(struct mci_host *mci, struct device *dev)
{
	struct bcm2835_host *host = to_bcm2835_host(mci);
	u32 temp;

	writel(SDVDD_POWER_OFF, host->regs + SDVDD);
	writel(0, host->regs + SDCMD);
	writel(0, host->regs + SDARG);
	/* Set timeout to a big enough value so we don't hit it */
	writel(0xf00000, host->regs + SDTOUT);
	writel(0, host->regs + SDCDIV);
	/* Clear status register */
	writel(SDHSTS_CLEAR_MASK, host->regs + SDHSTS);
	writel(0, host->regs + SDHCFG);
	writel(0, host->regs + SDHBCT);
	writel(0, host->regs + SDHBLC);

	/* Limit fifo usage due to silicon bug */
	temp = readl(host->regs + SDEDM);
	temp &= ~((SDEDM_THRESHOLD_MASK << SDEDM_READ_THRESHOLD_SHIFT) |
		  (SDEDM_THRESHOLD_MASK << SDEDM_WRITE_THRESHOLD_SHIFT));
	temp |= (FIFO_READ_THRESHOLD << SDEDM_READ_THRESHOLD_SHIFT) |
		(FIFO_WRITE_THRESHOLD << SDEDM_WRITE_THRESHOLD_SHIFT);
	writel(temp, host->regs + SDEDM);
	/* Wait for FIFO threshold to populate */
	mdelay(20);
	writel(SDVDD_POWER_ON, host->regs + SDVDD);
	/* Wait for all components to go through power on cycle */
	mdelay(20);
	writel(0, host->regs + SDHCFG);
	writel(0, host->regs + SDCDIV);

	return 0;
}

static int bcm2835_wait_transfer_complete(struct bcm2835_host *host)
{
	uint64_t start = get_time_ns();

	while (1) {
		u32 edm, fsm;

		edm = readl(host->regs + SDEDM);
		fsm = edm & SDEDM_FSM_MASK;

		if ((fsm == SDEDM_FSM_IDENTMODE) ||
		    (fsm == SDEDM_FSM_DATAMODE))
			break;

		if ((fsm == SDEDM_FSM_READWAIT) ||
		    (fsm == SDEDM_FSM_WRITESTART1) ||
		    (fsm == SDEDM_FSM_READDATA)) {
			writel(edm | SDEDM_FORCE_DATA_MODE,
			       host->regs + SDEDM);
			break;
		}

		/* Error out after 1 second */
		if (is_timeout(start, 1 * SECOND)) {
			dev_err(host->mci.hw_dev,
				"wait_transfer_complete - still waiting 1s\n");
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static int bcm2835_transfer_block_pio(struct bcm2835_host *host,
				      struct mci_data *data, unsigned int block,
				      bool is_read)
{
	u32 *buf = is_read ? (u32 *)data->dest : (u32 *)data->src;
	int copy_words = data->blocksize / sizeof(u32);
	uint64_t start = get_time_ns();

	if (data->blocksize % sizeof(u32))
		return -EINVAL;

	buf += (block * data->blocksize / sizeof(u32));

	/* Copy all contents from/to the FIFO as far as it reaches. */
	while (copy_words) {
		int fifo_words;
		u32 edm;

		if (is_timeout(start, 100 * MSECOND)) {
			dev_err(host->mci.hw_dev,
				"transfer_block_pio timeout\n");
			return -ETIMEDOUT;
		}

		edm = readl(host->regs + SDEDM);
		if (is_read)
			fifo_words = edm_fifo_fill(edm);
		else
			fifo_words = SDDATA_FIFO_WORDS - edm_fifo_fill(edm);

		if (fifo_words > copy_words)
			fifo_words = copy_words;

		/* Copy current chunk to/from the FIFO */
		while (fifo_words) {
			if (is_read)
				*(buf++) = readl(host->regs + SDDATA);
			else
				writel(*(buf++), host->regs + SDDATA);
			fifo_words--;
			copy_words--;
		}
	}

	return 0;
}

static int bcm2835_transfer_pio(struct bcm2835_host *host,
				struct mci_data *data)
{
	u32 sdhsts;
	bool is_read = !!(data->flags & MMC_DATA_READ);
	unsigned int block = 0;
	int ret = 0;

	while (block < data->blocks) {
		ret = bcm2835_transfer_block_pio(host, data, block, is_read);
		if (ret)
			return ret;

		sdhsts = readl(host->regs + SDHSTS);
		if (sdhsts & (SDHSTS_CRC16_ERROR |
			      SDHSTS_CRC7_ERROR |
			      SDHSTS_FIFO_ERROR)) {
			dev_err(host->mci.hw_dev,
				"%s transfer error - HSTS %08x\n",
				is_read ? "read" : "write", sdhsts);
			ret =  -EILSEQ;
		} else if ((sdhsts & (SDHSTS_CMD_TIME_OUT |
				      SDHSTS_REW_TIME_OUT))) {
			dev_err(host->mci.hw_dev,
				"%s timeout error - HSTS %08x\n",
				is_read ? "read" : "write", sdhsts);
			ret = -ETIMEDOUT;
		}
		block++;
	}

	return ret;
}

static u32 bcm2835_read_wait_sdcmd(struct bcm2835_host *host)
{
	u32 value;
	int ret;
	int timeout_us = SDHST_TIMEOUT_MAX_USEC;

	ret = readl_poll_timeout(host->regs + SDCMD, value,
				 !(value & SDCMD_NEW_FLAG), timeout_us);
	if (ret == -ETIMEDOUT)
		dev_err(host->mci.hw_dev, "%s: timeout (%d us)\n",
			__func__, timeout_us);

	return value;
}

static int bcm2835_send_command(struct bcm2835_host *host, struct mci_cmd *cmd,
				struct mci_data *data)
{
	u32 sdcmd, sdhsts;

	if ((cmd->resp_type & MMC_RSP_136) && (cmd->resp_type & MMC_RSP_BUSY)) {
		dev_err(host->mci.hw_dev, "unsupported response type!\n");
		return -EINVAL;
	}

	sdcmd = bcm2835_read_wait_sdcmd(host);
	if (sdcmd & SDCMD_NEW_FLAG) {
		dev_err(host->mci.hw_dev, "previous command never completed.\n");
		return -EBUSY;
	}

	/* Clear any error flags */
	sdhsts = readl(host->regs + SDHSTS);
	if (sdhsts & SDHSTS_ERROR_MASK)
		writel(sdhsts, host->regs + SDHSTS);

	if (data) {
		writel(data->blocksize, host->regs + SDHBCT);
		writel(data->blocks, host->regs + SDHBLC);
	}

	writel(cmd->cmdarg, host->regs + SDARG);

	sdcmd = cmd->cmdidx & SDCMD_CMD_MASK;

	if (!(cmd->resp_type & MMC_RSP_PRESENT)) {
		sdcmd |= SDCMD_NO_RESPONSE;
	} else {
		if (cmd->resp_type & MMC_RSP_136)
			sdcmd |= SDCMD_LONG_RESPONSE;
		if (cmd->resp_type & MMC_RSP_BUSY)
			sdcmd |= SDCMD_BUSYWAIT;
	}

	if (data) {
		if (data->flags & MMC_DATA_WRITE)
			sdcmd |= SDCMD_WRITE_CMD;
		if (data->flags & MMC_DATA_READ)
			sdcmd |= SDCMD_READ_CMD;
	}

	writel(sdcmd | SDCMD_NEW_FLAG, host->regs + SDCMD);

	return 0;
}

static int bcm2835_finish_command(struct bcm2835_host *host,
				  struct mci_cmd *cmd)
{
	u32 sdcmd;
	int ret = 0;

	sdcmd = bcm2835_read_wait_sdcmd(host);

	/* Check for errors */
	if (sdcmd & SDCMD_NEW_FLAG) {
		dev_err(host->mci.hw_dev, "command never completed.\n");
		return -EIO;
	} else if (sdcmd & SDCMD_FAIL_FLAG) {
		u32 sdhsts = readl(host->regs + SDHSTS);

		/* Clear the errors */
		writel(SDHSTS_ERROR_MASK, host->regs + SDHSTS);

		if (!(sdhsts & SDHSTS_CRC7_ERROR) ||
		    (cmd->cmdidx != MMC_CMD_SEND_OP_COND)) {
			if (sdhsts & SDHSTS_CMD_TIME_OUT) {
				ret = -ETIMEDOUT;
			} else {
				dev_err(host->mci.hw_dev,
					"unexpected command %d error\n",
					cmd->cmdidx);
				ret = -EILSEQ;
			}

			return ret;
		}
	}

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		if (cmd->resp_type & MMC_RSP_136) {
			int i;

			for (i = 0; i < 4; i++) {
				cmd->response[3 - i] =
					readl(host->regs + SDRSP0 + i * 4);
			}
		} else {
			cmd->response[0] = readl(host->regs + SDRSP0);
		}
	}

	return ret;
}

static int bcm2835_check_cmd_error(struct bcm2835_host *host, u32 intmask)
{
	int ret = -EINVAL;

	if (!(intmask & SDHSTS_ERROR_MASK))
		return 0;

	dev_err(host->mci.hw_dev, "sdhost_busy_irq: intmask %08x\n", intmask);
	if (intmask & SDHSTS_CRC7_ERROR) {
		ret = -EILSEQ;
	} else if (intmask & (SDHSTS_CRC16_ERROR |
			      SDHSTS_FIFO_ERROR)) {
		ret = -EILSEQ;
	} else if (intmask & (SDHSTS_REW_TIME_OUT | SDHSTS_CMD_TIME_OUT)) {
		ret = -ETIMEDOUT;
	}

	return ret;
}

static int bcm2835_check_data_error(struct bcm2835_host *host, u32 intmask)
{
	int ret = 0;

	if (intmask & (SDHSTS_CRC16_ERROR | SDHSTS_FIFO_ERROR))
		ret = -EILSEQ;
	if (intmask & SDHSTS_REW_TIME_OUT)
		ret = -ETIMEDOUT;

	if (ret)
		dev_err(host->mci.hw_dev, "data error %d\n", ret);

	return ret;
}

static int bcm2835_transmit(struct bcm2835_host *host, struct mci_cmd *cmd,
			    struct mci_data *data)
{
	u32 intmask = readl(host->regs + SDHSTS);
	int ret;

	/* Check for errors */
	if (data) {
		ret = bcm2835_check_data_error(host, intmask);
		if (ret)
			return ret;
	}

	ret = bcm2835_check_cmd_error(host, intmask);
	if (ret)
		return ret;

	/* Handle wait for busy end */
	if ((cmd->resp_type & MMC_RSP_BUSY) &&
	    (intmask & SDHSTS_BUSY_IRPT)) {
		writel(SDHSTS_BUSY_IRPT, host->regs + SDHSTS);
		bcm2835_finish_command(host, cmd);
	}

	/* Handle PIO data transfer */
	if (data) {
		ret = bcm2835_transfer_pio(host, data);
		if (ret)
			return ret;
		/* Transfer successful: wait for command to complete for real */
		ret = bcm2835_wait_transfer_complete(host);
	}

	return ret;
}

static void bcm2835_set_clock(struct bcm2835_host *host, unsigned int clock)
{
	int div;

	/* The SDCDIV register has 11 bits, and holds (div - 2).  But
	 * in data mode the max is 50MHz without a minimum, and only
	 * the bottom 3 bits are used. Since the switch over is
	 * automatic (unless we have marked the card as slow...),
	 * chosen values have to make sense in both modes. Ident mode
	 * must be 100-400KHz, so can range check the requested
	 * clock. CMD15 must be used to return to data mode, so this
	 * can be monitored.
	 *
	 * clock 250MHz -> 0->125MHz, 1->83.3MHz, 2->62.5MHz, 3->50.0MHz
	 *                 4->41.7MHz, 5->35.7MHz, 6->31.3MHz, 7->27.8MHz
	 *
	 *		 623->400KHz/27.8MHz
	 *		 reset value (507)->491159/50MHz
	 *
	 * BUT, the 3-bit clock divisor in data mode is too small if
	 * the core clock is higher than 250MHz, so instead use the
	 * SLOW_CARD configuration bit to force the use of the ident
	 * clock divisor at all times.
	 */

	if (clock < 100000) {
		/* Can't stop the clock, but make it as slow as possible
		 * to show willing
		 */
		writel(SDCDIV_MAX_CDIV, host->regs + SDCDIV);
		return;
	}

	div = host->mci.f_max / clock;
	if (div < 2)
		div = 2;
	if ((host->mci.f_max / div) > clock)
		div++;
	div -= 2;

	if (div > SDCDIV_MAX_CDIV)
		div = SDCDIV_MAX_CDIV;

	clock = host->mci.f_max / (div + 2);

	writel(div, host->regs + SDCDIV);

	/* Set the timeout to 500ms */
	writel(clock / 2, host->regs + SDTOUT);
}

static int bcm2835_send_cmd(struct mci_host *mci, struct mci_cmd *cmd,
			    struct mci_data *data)
{
	struct bcm2835_host *host = to_bcm2835_host(mci);
	u32 edm, fsm;
	int ret = 0;

	if (data && !is_power_of_2(data->blocksize)) {
		dev_err(mci->hw_dev, "unsupported block size (%d bytes)\n",
			data->blocksize);
		return -EINVAL;
	}

	edm = readl(host->regs + SDEDM);
	fsm = edm & SDEDM_FSM_MASK;

	if ((fsm != SDEDM_FSM_IDENTMODE) &&
	    (fsm != SDEDM_FSM_DATAMODE) &&
	    (cmd->cmdidx != MMC_CMD_STOP_TRANSMISSION)) {
		dev_err(mci->hw_dev,
			"previous command (%d) not complete (EDM %08x)\n",
			readl(host->regs + SDCMD) & SDCMD_CMD_MASK, edm);

		return -EILSEQ;
	}

	ret = bcm2835_send_command(host, cmd, data);
	if (ret)
		return ret;

	if (!(cmd->resp_type & MMC_RSP_BUSY)) {
		ret = bcm2835_finish_command(host, cmd);
		if (ret)
			return ret;
	}

	/* Wait for completion of busy signal or data transfer */
	if ((cmd->resp_type & MMC_RSP_BUSY) || data)
		ret = bcm2835_transmit(host, cmd, data);

	return ret;
}

static void bcm2835_set_ios(struct mci_host *mci, struct mci_ios *ios)
{
	struct bcm2835_host *host = to_bcm2835_host(mci);
	u32 hcfg = SDHCFG_WIDE_INT_BUS | SDHCFG_SLOW_CARD;

	if (ios->clock)
		bcm2835_set_clock(host, ios->clock);

	/* set bus width */
	if (ios->bus_width == MMC_BUS_WIDTH_4)
		hcfg |= SDHCFG_WIDE_EXT_BUS;

	writel(hcfg, host->regs + SDHCFG);
}

static int bcm2835_sdhost_probe(struct device *dev)
{
	struct bcm2835_host *host;
	struct resource *iores;
	struct mci_host *mci;

	host = xzalloc(sizeof(*host));
	mci = &host->mci;

	host->clk = clk_get(dev, NULL);
	if (IS_ERR(host->clk))
		return PTR_ERR(host->clk);

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "could not get iomem region\n");
		return PTR_ERR(iores);
	}
	host->regs = IOMEM(iores->start);

	mci->hw_dev = dev;
	mci->f_max = clk_get_rate(host->clk);
	mci->f_min = mci->f_max / SDCDIV_MAX_CDIV;
	mci->voltages = MMC_VDD_32_33 | MMC_VDD_33_34;
	mci->host_caps |= MMC_CAP_MMC_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED_52MHZ |
			  MMC_CAP_SD_HIGHSPEED;

	mci->init = bcm2835_sdhost_init;
	mci->set_ios = bcm2835_set_ios;
	mci->send_cmd = bcm2835_send_cmd;

	mci_of_parse(mci);

	return mci_register(mci);
}

static __maybe_unused struct of_device_id bcm2835_sdhost_compatible[] = {
	{ .compatible = "brcm,bcm2835-sdhost" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, bcm2835_sdhost_compatible);

static struct driver bcm2835_sdhost_driver = {
	.name  = "bcm2835-sdhost",
	.probe = bcm2835_sdhost_probe,
	.of_compatible = DRV_OF_COMPAT(bcm2835_sdhost_compatible),
};
device_platform_driver(bcm2835_sdhost_driver);
