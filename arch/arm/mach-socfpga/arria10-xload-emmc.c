#include <common.h>
#include <init.h>
#include <linux/sizes.h>
#include <mach/generic.h>
#include <mach/arria10-regs.h>
#include <mach/arria10-system-manager.h>
#include <mach/arria10-xload.h>
#include <mci.h>
#include "../../../drivers/mci/sdhci.h"
#include "../../../drivers/mci/dw_mmc.h"

#define SECTOR_SIZE 512

static int dwmci_wait_reset(uint32_t value)
{
	uint32_t ctrl;
	int32_t timeout;

	timeout = 10000;

	writel(value, ARRIA10_SDMMC_ADDR + DWMCI_CTRL);

	while (timeout-- > 0) {
		ctrl = readl(ARRIA10_SDMMC_ADDR + DWMCI_CTRL);
		if (!(ctrl & DWMCI_RESET_ALL))
			return 0;
	}

	return -EIO;
}

static int dwmci_prepare_data(struct mci_data *data)
{
	unsigned long ctrl;

	dwmci_wait_reset(DWMCI_CTRL_FIFO_RESET);

	writel(DWMCI_INTMSK_TXDR | DWMCI_INTMSK_RXDR,
	       ARRIA10_SDMMC_ADDR + DWMCI_RINTSTS);

	ctrl = readl(ARRIA10_SDMMC_ADDR + DWMCI_INTMASK);
	ctrl |= DWMCI_INTMSK_TXDR | DWMCI_INTMSK_RXDR;
	writel(ctrl, ARRIA10_SDMMC_ADDR + DWMCI_INTMASK);

	ctrl = readl(ARRIA10_SDMMC_ADDR + DWMCI_CTRL);
	ctrl &= ~(DWMCI_IDMAC_EN | DWMCI_DMA_EN);
	writel(ctrl, ARRIA10_SDMMC_ADDR + DWMCI_CTRL);

	writel(0x1, ARRIA10_SDMMC_ADDR + DWMCI_FIFOTH);
	writel(0xffffffff, ARRIA10_SDMMC_ADDR + DWMCI_TMOUT);
	writel(0x0, ARRIA10_SDMMC_ADDR + DWMCI_IDINTEN);

	return 0;
}

static int dwmci_read_data_pio(struct mci_data *data)
{
	u32 *pdata = (u32 *)data->dest;
	u32 val, status, timeout;
	u32 rcnt, rlen = 0;

	for (rcnt = (data->blocksize * data->blocks)>>2; rcnt; rcnt--) {
		timeout = 20000;
		status = readl(ARRIA10_SDMMC_ADDR + DWMCI_STATUS);
		while (--timeout > 0
		       && (status & DWMCI_STATUS_FIFO_EMPTY)) {
			__udelay(200);
			status = readl(ARRIA10_SDMMC_ADDR + DWMCI_STATUS);
		}
		if (!timeout)
			break;

		val = readl(ARRIA10_SDMMC_ADDR + DWMCI_DATA);
		*pdata++ = val;
		rlen += 4;
	}
	writel(DWMCI_INTMSK_RXDR, ARRIA10_SDMMC_ADDR + DWMCI_RINTSTS);

	return rlen;
}

static int dwmci_cmd(struct mci_cmd *cmd, struct mci_data *data)
{
	int flags = 0;
	uint32_t mask;
	int timeout;

	timeout = 100000;
	while (readl(ARRIA10_SDMMC_ADDR + DWMCI_STATUS) & DWMCI_STATUS_BUSY) {
		if (timeout-- <= 0)
			return -ETIMEDOUT;

	}

	writel(DWMCI_INTMSK_ALL, ARRIA10_SDMMC_ADDR + DWMCI_RINTSTS);

	if (data) {
		writel(data->blocksize, ARRIA10_SDMMC_ADDR + DWMCI_BLKSIZ);
		writel(data->blocksize * data->blocks, ARRIA10_SDMMC_ADDR +
		       DWMCI_BYTCNT);

		dwmci_prepare_data(data);
	}

	writel(cmd->cmdarg, ARRIA10_SDMMC_ADDR + DWMCI_CMDARG);

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

	writel(flags, ARRIA10_SDMMC_ADDR + DWMCI_CMD);

	for (timeout = 10000; timeout > 0; timeout--) {
		mask = readl(ARRIA10_SDMMC_ADDR + DWMCI_RINTSTS);
		if (mask & DWMCI_INTMSK_CDONE) {
			if (!data)
				writel(mask, ARRIA10_SDMMC_ADDR + DWMCI_RINTSTS);
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
			cmd->response[0] = readl(ARRIA10_SDMMC_ADDR + DWMCI_RESP3);
			cmd->response[1] = readl(ARRIA10_SDMMC_ADDR + DWMCI_RESP2);
			cmd->response[2] = readl(ARRIA10_SDMMC_ADDR + DWMCI_RESP1);
			cmd->response[3] = readl(ARRIA10_SDMMC_ADDR + DWMCI_RESP0);
		} else {
			cmd->response[0] = readl(ARRIA10_SDMMC_ADDR + DWMCI_RESP0);
		}
	}

	if (data) {
		do {
			mask = readl(ARRIA10_SDMMC_ADDR + DWMCI_RINTSTS);
			if (mask & (DWMCI_DATA_ERR))
				return -EIO;

			if (mask & DWMCI_INTMSK_RXDR) {
				dwmci_read_data_pio(data);
				mask = readl(ARRIA10_SDMMC_ADDR + DWMCI_RINTSTS);
			}
		} while (!(mask & DWMCI_INTMSK_DTO));

		writel(mask, ARRIA10_SDMMC_ADDR + DWMCI_RINTSTS);
	}

	return 0;
}

int arria10_read_blocks(void *dst, int blocknum, size_t len)
{
	struct mci_cmd cmd;
	struct mci_data data;
	int ret;
	int blocks;

	blocks = len / SECTOR_SIZE;

	if (blocks > 1)
		cmd.cmdidx = MMC_CMD_READ_MULTIPLE_BLOCK;
	else
		cmd.cmdidx = MMC_CMD_READ_SINGLE_BLOCK;

	cmd.cmdarg = blocknum;
	cmd.resp_type = MMC_RSP_R1;

	data.dest = dst;
	data.blocks = blocks;
	data.blocksize = SECTOR_SIZE;
	data.flags = MMC_DATA_READ;

	ret = dwmci_cmd(&cmd, &data);

	if (ret || blocks > 1) {
		cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
		cmd.cmdarg = 0;
		cmd.resp_type = MMC_RSP_R1b;

		dwmci_cmd(&cmd, NULL);
	}

	return ret;
}

void arria10_init_mmc(void)
{
	writel(ARRIA10_SYSMGR_SDMMC_DRVSEL(3) |
	       ARRIA10_SYSMGR_SDMMC_SMPLSEL(2),
	       ARRIA10_SYSMGR_SDMMC);

	/* enable power to card */
	writel(0x1, ARRIA10_SDMMC_ADDR + DWMCI_PWREN);

	writel(DWMCI_CTYPE_1BIT, ARRIA10_SDMMC_ADDR + DWMCI_CTYPE);
}
