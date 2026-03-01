// SPDX-License-Identifier: GPL-2.0-only

#include <linux/sizes.h>
#include <io.h>
#include <mci.h>
#include <pbl/bio.h>
#include <pbl/mci.h>

#include "dw_mmc.h"

#define SECTOR_SIZE 512

struct dw_mmc {
	void __iomem *base;
};

static inline void __udelay(unsigned us)
{
	volatile unsigned int i;

	for (i = 0; i < us * 3; i++);
}

static int dwmci_wait_reset(struct dw_mmc *dw_mmc, uint32_t value)
{
	void __iomem *base = dw_mmc->base;
	uint32_t ctrl;
	int32_t timeout;

	timeout = 10000;

	writel(value, base + DWMCI_CTRL);

	while (timeout-- > 0) {
		ctrl = readl(base + DWMCI_CTRL);
		if (!(ctrl & DWMCI_RESET_ALL))
			return 0;
	}

	return -EIO;
}

static int dwmci_prepare_data(struct dw_mmc *dw_mmc, struct mci_data *data)
{
	void __iomem *base = dw_mmc->base;
	unsigned long ctrl;

	dwmci_wait_reset(dw_mmc, DWMCI_CTRL_FIFO_RESET);

	writel(DWMCI_INTMSK_TXDR | DWMCI_INTMSK_RXDR,
	       base + DWMCI_RINTSTS);

	ctrl = readl(base + DWMCI_INTMASK);
	ctrl |= DWMCI_INTMSK_TXDR | DWMCI_INTMSK_RXDR;
	writel(ctrl, base + DWMCI_INTMASK);

	ctrl = readl(base + DWMCI_CTRL);
	ctrl &= ~(DWMCI_IDMAC_EN | DWMCI_DMA_EN);
	writel(ctrl, base + DWMCI_CTRL);

	writel(0x1, base + DWMCI_FIFOTH);
	writel(0xffffffff, base + DWMCI_TMOUT);
	writel(0x0, base + DWMCI_IDINTEN);

	return 0;
}

static int dwmci_read_data_pio(struct dw_mmc *dw_mmc, struct mci_data *data)
{
	void __iomem *base = dw_mmc->base;
	u32 *pdata = (u32 *)data->dest;
	u32 val, status, timeout;
	u32 rcnt, rlen = 0;

	for (rcnt = (data->blocksize * data->blocks)>>2; rcnt; rcnt--) {
		timeout = 20000;
		status = readl(base + DWMCI_STATUS);
		while (--timeout > 0
		       && (status & DWMCI_STATUS_FIFO_EMPTY)) {
			__udelay(200);
			status = readl(base + DWMCI_STATUS);
		}
		if (!timeout)
			break;

		val = readl(base + DWMCI_DATA);
		*pdata++ = val;
		rlen += 4;
	}
	writel(DWMCI_INTMSK_RXDR, base + DWMCI_RINTSTS);

	return rlen;
}

static int pbl_dw_mmc_send_cmd(struct pbl_mci *mci,
			       struct mci_cmd *cmd, struct mci_data *data)
{
	struct dw_mmc *dw_mmc = mci->priv;
	void __iomem *base = dw_mmc->base;
	int flags = 0;
	uint32_t mask;
	int timeout;

	timeout = 100000;
	while (readl(base + DWMCI_STATUS) & DWMCI_STATUS_BUSY) {
		if (timeout-- <= 0)
			return -ETIMEDOUT;
	}

	writel(DWMCI_INTMSK_ALL, base + DWMCI_RINTSTS);

	if (data) {
		writel(data->blocksize, base + DWMCI_BLKSIZ);
		writel(data->blocksize * data->blocks, base +
		       DWMCI_BYTCNT);

		dwmci_prepare_data(dw_mmc, data);
	}

	writel(cmd->cmdarg, base + DWMCI_CMDARG);

	if (data)
		flags = DWMCI_CMD_DATA_EXP;

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

	writel(flags, base + DWMCI_CMD);

	for (timeout = 10000; timeout > 0; timeout--) {
		mask = readl(base + DWMCI_RINTSTS);
		if (mask & DWMCI_INTMSK_CDONE) {
			if (!data)
				writel(mask, base + DWMCI_RINTSTS);
			break;
		}
	}

	if (timeout <= 0)
		return -ETIMEDOUT;

	if (mask & DWMCI_INTMSK_RTO)
		return -ETIMEDOUT;
	else if (mask & DWMCI_INTMSK_RE)
		return -EIO;

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		if (cmd->resp_type & MMC_RSP_136) {
			cmd->response[0] = readl(base + DWMCI_RESP3);
			cmd->response[1] = readl(base + DWMCI_RESP2);
			cmd->response[2] = readl(base + DWMCI_RESP1);
			cmd->response[3] = readl(base + DWMCI_RESP0);
		} else {
			cmd->response[0] = readl(base + DWMCI_RESP0);
		}
	}

	if (data) {
		do {
			mask = readl(base + DWMCI_RINTSTS);
			if (mask & (DWMCI_DATA_ERR))
				return -EIO;

			if (mask & DWMCI_INTMSK_RXDR) {
				dwmci_read_data_pio(dw_mmc, data);
				mask = readl(base + DWMCI_RINTSTS);
			}
		} while (!(mask & DWMCI_INTMSK_DTO));

		writel(mask, base + DWMCI_RINTSTS);
	}

	return 0;
}

static struct pbl_mci mci;
static struct dw_mmc dw_mmc;

int dw_mmc_pbl_bio_init(struct pbl_bio *bio, void __iomem *dw_mmc_base)
{
	dw_mmc.base = dw_mmc_base;

	/* enable power to card */
	writel(0x1, dw_mmc_base + DWMCI_PWREN);

	mci.priv = &dw_mmc;
	mci.send_cmd = pbl_dw_mmc_send_cmd;

	return pbl_mci_bio_init(&mci, bio);
}
