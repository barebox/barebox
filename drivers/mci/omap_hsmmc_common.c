// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2008 Texas Instruments (http://www.ti.com/, Sukumar Ghorai <s-ghorai@ti.com>)

/* #define DEBUG */
#include <config.h>
#include <common.h>
#include <init.h>
#include <driver.h>
#include <mci.h>
#include <clock.h>
#include <errno.h>
#include <io.h>
#include <linux/err.h>

#include "omap_hsmmc.h"

struct hsmmc {
	unsigned char res1[0x10];
	unsigned int sysconfig;		/* 0x10 */
	unsigned int sysstatus;		/* 0x14 */
	unsigned char res2[0x14];
	unsigned int con;		/* 0x2C */
	unsigned char res3[0xD4];
	unsigned int blk;		/* 0x104 */
	unsigned int arg;		/* 0x108 */
	unsigned int cmd;		/* 0x10C */
	unsigned int rsp10;		/* 0x110 */
	unsigned int rsp32;		/* 0x114 */
	unsigned int rsp54;		/* 0x118 */
	unsigned int rsp76;		/* 0x11C */
	unsigned int data;		/* 0x120 */
	unsigned int pstate;		/* 0x124 */
	unsigned int hctl;		/* 0x128 */
	unsigned int sysctl;		/* 0x12C */
	unsigned int stat;		/* 0x130 */
	unsigned int ie;		/* 0x134 */
	unsigned char res4[0x8];
	unsigned int capa;		/* 0x140 */
};

/*
 * OMAP HS MMC Bit definitions
 */
#define MMC_SOFTRESET			(0x1 << 1)
#define RESETDONE			(0x1 << 0)
#define NOOPENDRAIN			(0x0 << 0)
#define OPENDRAIN			(0x1 << 0)
#define OD				(0x1 << 0)
#define INIT_NOINIT			(0x0 << 1)
#define INIT_INITSTREAM			(0x1 << 1)
#define HR_NOHOSTRESP			(0x0 << 2)
#define STR_BLOCK			(0x0 << 3)
#define MODE_FUNC			(0x0 << 4)
#define DW8_1_4BITMODE			(0x0 << 5)
#define MIT_CTO				(0x0 << 6)
#define CDP_ACTIVEHIGH			(0x0 << 7)
#define WPP_ACTIVEHIGH			(0x0 << 8)
#define RESERVED_MASK			(0x3 << 9)
#define CTPL_MMC_SD			(0x0 << 11)
#define BLEN_512BYTESLEN		(0x200 << 0)
#define NBLK_STPCNT			(0x0 << 16)
#define DE_DISABLE			(0x0 << 0)
#define BCE_DISABLE			(0x0 << 1)
#define BCE_ENABLE			(0x1 << 1)
#define ACEN_DISABLE			(0x0 << 2)
#define DDIR_OFFSET			(4)
#define DDIR_MASK			(0x1 << 4)
#define DDIR_WRITE			(0x0 << 4)
#define DDIR_READ			(0x1 << 4)
#define MSBS_SGLEBLK			(0x0 << 5)
#define MSBS_MULTIBLK			(0x1 << 5)
#define RSP_TYPE_OFFSET			(16)
#define RSP_TYPE_MASK			(0x3 << 16)
#define RSP_TYPE_NORSP			(0x0 << 16)
#define RSP_TYPE_LGHT136		(0x1 << 16)
#define RSP_TYPE_LGHT48			(0x2 << 16)
#define RSP_TYPE_LGHT48B		(0x3 << 16)
#define CCCE_NOCHECK			(0x0 << 19)
#define CCCE_CHECK			(0x1 << 19)
#define CICE_NOCHECK			(0x0 << 20)
#define CICE_CHECK			(0x1 << 20)
#define DP_OFFSET			(21)
#define DP_MASK				(0x1 << 21)
#define DP_NO_DATA			(0x0 << 21)
#define DP_DATA				(0x1 << 21)
#define CMD_TYPE_NORMAL			(0x0 << 22)
#define INDEX_OFFSET			(24)
#define INDEX_MASK			(0x3f << 24)
#define INDEX(i)			(i << 24)
#define DATI_MASK			(0x1 << 1)
#define DATI_CMDDIS			(0x1 << 1)
#define DTW_1_BITMODE			(0x0 << 1)
#define DTW_4_BITMODE			(0x1 << 1)
#define DTW_8_BITMODE                   (0x1 << 5) /* CON[DW8]*/
#define SDBP_PWROFF			(0x0 << 8)
#define SDBP_PWRON			(0x1 << 8)
#define SDVS_1V8			(0x5 << 9)
#define SDVS_3V0			(0x6 << 9)
#define ICE_MASK			(0x1 << 0)
#define ICE_STOP			(0x0 << 0)
#define ICS_MASK			(0x1 << 1)
#define ICS_NOTREADY			(0x0 << 1)
#define ICE_OSCILLATE			(0x1 << 0)
#define CEN_MASK			(0x1 << 2)
#define CEN_DISABLE			(0x0 << 2)
#define CEN_ENABLE			(0x1 << 2)
#define CLKD_OFFSET			(6)
#define CLKD_MASK			(0x3FF << 6)
#define DTO_MASK			(0xF << 16)
#define DTO_15THDTO			(0xE << 16)
#define SOFTRESETALL			(0x1 << 24)
#define CC_MASK				(0x1 << 0)
#define TC_MASK				(0x1 << 1)
#define BWR_MASK			(0x1 << 4)
#define BRR_MASK			(0x1 << 5)
#define ERRI_MASK			(0x1 << 15)
#define IE_CC				(0x01 << 0)
#define IE_TC				(0x01 << 1)
#define IE_BWR				(0x01 << 4)
#define IE_BRR				(0x01 << 5)
#define IE_CTO				(0x01 << 16)
#define IE_CCRC				(0x01 << 17)
#define IE_CEB				(0x01 << 18)
#define IE_CIE				(0x01 << 19)
#define IE_DTO				(0x01 << 20)
#define IE_DCRC				(0x01 << 21)
#define IE_DEB				(0x01 << 22)
#define IE_CERR				(0x01 << 28)
#define IE_BADA				(0x01 << 29)

#define VS30_3V0SUP			(1 << 25)
#define VS18_1V8SUP			(1 << 26)

/* Driver definitions */
#define MMCSD_SECTOR_SIZE		512
#define MMC_CARD			0
#define SD_CARD				1
#define BYTE_MODE			0
#define SECTOR_MODE			1
#define CLK_INITSEQ			0
#define CLK_400KHZ			1
#define CLK_MISC			2

#define RSP_TYPE_NONE	(RSP_TYPE_NORSP   | CCCE_NOCHECK | CICE_NOCHECK)
#define MMC_CMD0	(INDEX(0)  | RSP_TYPE_NONE | DP_NO_DATA | DDIR_WRITE)

/* Clock Configurations and Macros */
#define MMC_CLOCK_REFERENCE	96 /* MHz */

#define mmc_reg_out(addr, mask, val)\
	writel((readl(addr) & (~(mask))) | ((val) & (mask)), (addr))

#define to_hsmmc(mci)	container_of(mci, struct omap_hsmmc, mci)

static int mmc_init_stream(struct omap_hsmmc *hsmmc)
{
	uint64_t start;
	struct hsmmc *mmc_base = hsmmc->base;

	writel(readl(&mmc_base->con) | INIT_INITSTREAM, &mmc_base->con);

	writel(MMC_CMD0, &mmc_base->cmd);
	start = get_time_ns();
	while (!(readl(&mmc_base->stat) & CC_MASK)) {
		if (is_timeout(start, SECOND)) {
			dev_dbg(hsmmc->dev, "timedout waiting for cc!\n");
			return -ETIMEDOUT;
		}
	}
	writel(CC_MASK, &mmc_base->stat);
	writel(MMC_CMD0, &mmc_base->cmd);

	start = get_time_ns();
	while (!(readl(&mmc_base->stat) & CC_MASK)) {
		if (is_timeout(start, SECOND)) {
			dev_dbg(hsmmc->dev, "timedout waiting for cc2!\n");
			return -ETIMEDOUT;
		}
	}
	writel(readl(&mmc_base->con) & ~INIT_INITSTREAM, &mmc_base->con);

	return 0;
}

int omap_hsmmc_init(struct omap_hsmmc *hsmmc)
{
	struct hsmmc *mmc_base = hsmmc->base;
	unsigned int reg_val;
	unsigned int dsor;
	uint64_t start;

	writel(readl(&mmc_base->sysconfig) | MMC_SOFTRESET,
		&mmc_base->sysconfig);

	start = get_time_ns();
	while ((readl(&mmc_base->sysstatus) & RESETDONE) == 0) {
		if (is_timeout(start, SECOND)) {
			dev_dbg(hsmmc->dev, "timeout waiting for reset done\n");
			return -ETIMEDOUT;
		}
	}

	writel(readl(&mmc_base->sysctl) | SOFTRESETALL, &mmc_base->sysctl);

	start = get_time_ns();
	while ((readl(&mmc_base->sysctl) & SOFTRESETALL) != 0x0) {
		if (is_timeout(start, SECOND)) {
			dev_dbg(hsmmc->dev, "timedout waiting for softresetall!\n");
			return -ETIMEDOUT;
		}
	}

	writel(DTW_1_BITMODE | SDBP_PWROFF | SDVS_3V0, &mmc_base->hctl);
	writel(readl(&mmc_base->capa) | VS30_3V0SUP | VS18_1V8SUP,
		&mmc_base->capa);

	reg_val = readl(&mmc_base->con) & RESERVED_MASK;

	writel(CTPL_MMC_SD | reg_val | WPP_ACTIVEHIGH | CDP_ACTIVEHIGH |
		MIT_CTO | DW8_1_4BITMODE | MODE_FUNC | STR_BLOCK |
		HR_NOHOSTRESP | INIT_NOINIT | NOOPENDRAIN, &mmc_base->con);

	dsor = 240;
	mmc_reg_out(&mmc_base->sysctl, (ICE_MASK | DTO_MASK | CEN_MASK),
		(ICE_STOP | DTO_15THDTO | CEN_DISABLE));
	mmc_reg_out(&mmc_base->sysctl, ICE_MASK | CLKD_MASK,
		(dsor << CLKD_OFFSET) | ICE_OSCILLATE);

	start = get_time_ns();
	while ((readl(&mmc_base->sysctl) & ICS_MASK) == ICS_NOTREADY) {
		if (is_timeout(start, SECOND)) {
			dev_dbg(hsmmc->dev, "timedout waiting for ics!\n");
			return -ETIMEDOUT;
		}
	}

	writel(readl(&mmc_base->sysctl) | CEN_ENABLE, &mmc_base->sysctl);

	writel(readl(&mmc_base->hctl) | SDBP_PWRON, &mmc_base->hctl);

	writel(IE_BADA | IE_CERR | IE_DEB | IE_DCRC | IE_DTO | IE_CIE |
		IE_CEB | IE_CCRC | IE_CTO | IE_BRR | IE_BWR | IE_TC | IE_CC,
		&mmc_base->ie);

	return mmc_init_stream(hsmmc);
}

static int mmc_read_data(struct omap_hsmmc *hsmmc, char *buf, unsigned int size)
{
	struct hsmmc *mmc_base = hsmmc->base;
	unsigned int *output_buf = (unsigned int *)buf;
	unsigned int mmc_stat;
	unsigned int count;

	/*
	 * Start Polled Read
	 */
	count = (size > MMCSD_SECTOR_SIZE) ? MMCSD_SECTOR_SIZE : size;
	count /= 4;

	while (size) {
		uint64_t start = get_time_ns();
		do {
			mmc_stat = readl(&mmc_base->stat);
			if (is_timeout(start, SECOND)) {
				dev_err(hsmmc->dev, "timedout waiting for status!\n");
				return -ETIMEDOUT;
			}
		} while (mmc_stat == 0);

		if ((mmc_stat & ERRI_MASK) != 0) {
			dev_err(hsmmc->dev, "Error while reading data. status: 0x%08x\n",
				mmc_stat);
			return -EIO;
		}

		if (mmc_stat & BRR_MASK) {
			unsigned int k;

			writel(readl(&mmc_base->stat) | BRR_MASK,
				&mmc_base->stat);
			for (k = 0; k < count; k++) {
				*output_buf = readl(&mmc_base->data);
				output_buf++;
			}
			size -= (count*4);
		}

		if (mmc_stat & BWR_MASK)
			writel(readl(&mmc_base->stat) | BWR_MASK,
				&mmc_base->stat);

		if (mmc_stat & TC_MASK) {
			writel(readl(&mmc_base->stat) | TC_MASK,
				&mmc_base->stat);
			break;
		}
	}
	return 0;
}

static int mmc_write_data(struct omap_hsmmc *hsmmc, const char *buf, unsigned int size)
{
	struct hsmmc *mmc_base = hsmmc->base;
	unsigned int *input_buf = (unsigned int *)buf;
	unsigned int mmc_stat;
	unsigned int count;

	/*
	 * Start Polled Read
	 */
	count = (size > MMCSD_SECTOR_SIZE) ? MMCSD_SECTOR_SIZE : size;
	count /= 4;

	while (size) {
		uint64_t start = get_time_ns();
		do {
			mmc_stat = readl(&mmc_base->stat);
			if (is_timeout(start, SECOND)) {
				dev_dbg(hsmmc->dev, "timedout waiting for status!\n");
				return -ETIMEDOUT;
			}
		} while (mmc_stat == 0);

		if ((mmc_stat & ERRI_MASK) != 0) {
			dev_err(hsmmc->dev, "Error while reading data. status: 0x%08x\n",
				mmc_stat);
			return -EIO;
		}

		if (mmc_stat & BWR_MASK) {
			unsigned int k;

			writel(readl(&mmc_base->stat) | BWR_MASK,
					&mmc_base->stat);
			for (k = 0; k < count; k++) {
				writel(*input_buf, &mmc_base->data);
				input_buf++;
			}
			size -= (count * 4);
		}

		if (mmc_stat & BRR_MASK)
			writel(readl(&mmc_base->stat) | BRR_MASK,
				&mmc_base->stat);

		if (mmc_stat & TC_MASK) {
			writel(readl(&mmc_base->stat) | TC_MASK,
				&mmc_base->stat);
			break;
		}
	}
	return 0;
}

int omap_hsmmc_send_cmd(struct omap_hsmmc *hsmmc, struct mci_cmd *cmd,
			struct mci_data *data)
{
	struct hsmmc *mmc_base = hsmmc->base;
	unsigned int flags, mmc_stat;
	uint64_t start;

	start = get_time_ns();
	while ((readl(&mmc_base->pstate) & DATI_MASK) == DATI_CMDDIS) {
		if (is_timeout(start, SECOND)) {
			dev_dbg(hsmmc->dev, "timedout waiting for cmddis!\n");
			return -ETIMEDOUT;
		}
	}

	writel(0xFFFFFFFF, &mmc_base->stat);
	start = get_time_ns();
	while (readl(&mmc_base->stat)) {
		if (is_timeout(start, SECOND)) {
			dev_dbg(hsmmc->dev, "timedout waiting for stat!\n");
			return -ETIMEDOUT;
		}
	}

	/*
	 * CMDREG
	 * CMDIDX[13:8]	: Command index
	 * DATAPRNT[5]	: Data Present Select
	 * ENCMDIDX[4]	: Command Index Check Enable
	 * ENCMDCRC[3]	: Command CRC Check Enable
	 * RSPTYP[1:0]
	 *	00 = No Response
	 *	01 = Length 136
	 *	10 = Length 48
	 *	11 = Length 48 Check busy after response
	 */
	/* Delay added before checking the status of frq change
	 * retry not supported by mmc.c(core file)
	 */
	if (cmd->cmdidx == SD_CMD_APP_SEND_SCR)
		udelay(50000); /* wait 50 ms */

	if (!(cmd->resp_type & MMC_RSP_PRESENT))
		flags = 0;
	else if (cmd->resp_type & MMC_RSP_136)
		flags = RSP_TYPE_LGHT136 | CICE_NOCHECK;
	else if (cmd->resp_type & MMC_RSP_BUSY)
		flags = RSP_TYPE_LGHT48B;
	else
		flags = RSP_TYPE_LGHT48;

	/* enable default flags */
	flags =	flags | (CMD_TYPE_NORMAL | CICE_NOCHECK | CCCE_NOCHECK |
			MSBS_SGLEBLK | ACEN_DISABLE | BCE_DISABLE | DE_DISABLE);

	if (cmd->resp_type & MMC_RSP_CRC)
		flags |= CCCE_CHECK;
	if (cmd->resp_type & MMC_RSP_OPCODE)
		flags |= CICE_CHECK;

	if (data) {
		if ((cmd->cmdidx == MMC_CMD_READ_MULTIPLE_BLOCK) ||
			 (cmd->cmdidx == MMC_CMD_WRITE_MULTIPLE_BLOCK)) {
			flags |= (MSBS_MULTIBLK | BCE_ENABLE);
			data->blocksize = 512;
			writel(data->blocksize | (data->blocks << 16),
							&mmc_base->blk);
		} else
			writel(data->blocksize | NBLK_STPCNT, &mmc_base->blk);

		if (data->flags & MMC_DATA_READ)
			flags |= (DP_DATA | DDIR_READ);
		else
			flags |= (DP_DATA | DDIR_WRITE);
	}

	writel(cmd->cmdarg, &mmc_base->arg);
	writel((cmd->cmdidx << 24) | flags, &mmc_base->cmd);

	start = get_time_ns();
	do {
		mmc_stat = readl(&mmc_base->stat);
		if (is_timeout(start, SECOND)) {
			dev_dbg(hsmmc->dev, "timeout: No status update\n");
			return -ETIMEDOUT;
		}
	} while (!mmc_stat);

	if ((mmc_stat & IE_CTO) != 0)
		return -ETIMEDOUT;
	else if ((mmc_stat & ERRI_MASK) != 0)
		return -1;

	if (mmc_stat & CC_MASK) {
		writel(CC_MASK, &mmc_base->stat);
		if (cmd->resp_type & MMC_RSP_PRESENT) {
			if (cmd->resp_type & MMC_RSP_136) {
				/* response type 2 */
				cmd->response[3] = readl(&mmc_base->rsp10);
				cmd->response[2] = readl(&mmc_base->rsp32);
				cmd->response[1] = readl(&mmc_base->rsp54);
				cmd->response[0] = readl(&mmc_base->rsp76);
			} else
				/* response types 1, 1b, 3, 4, 5, 6 */
				cmd->response[0] = readl(&mmc_base->rsp10);
		}
	}

	if (!data)
		return 0;

	if (data->flags & MMC_DATA_READ)
		return mmc_read_data(hsmmc, data->dest,
				     data->blocksize * data->blocks);

	if (IS_ENABLED(CONFIG_MCI_WRITE))
		return mmc_write_data(hsmmc, data->src,
				      data->blocksize * data->blocks);

	return -ENOSYS;
}

void omap_hsmmc_set_ios(struct omap_hsmmc *hsmmc, struct mci_ios *ios)
{
	struct hsmmc *mmc_base = hsmmc->base;
	unsigned int dsor = 0;
	uint64_t start;

	/* configue bus width */
	switch (ios->bus_width) {
	case MMC_BUS_WIDTH_8:
		writel(readl(&mmc_base->con) | DTW_8_BITMODE,
			&mmc_base->con);
		break;

	case MMC_BUS_WIDTH_4:
		writel(readl(&mmc_base->con) & ~DTW_8_BITMODE,
			&mmc_base->con);
		writel(readl(&mmc_base->hctl) | DTW_4_BITMODE,
			&mmc_base->hctl);
		break;

	case MMC_BUS_WIDTH_1:
		writel(readl(&mmc_base->con) & ~DTW_8_BITMODE,
			&mmc_base->con);
		writel(readl(&mmc_base->hctl) & ~DTW_4_BITMODE,
			&mmc_base->hctl);
		break;
	default:
		return;
	}

	/* configure clock with 96Mhz system clock.
	 */
	if (ios->clock != 0) {
		dsor = (MMC_CLOCK_REFERENCE * 1000000 / ios->clock);
		if ((MMC_CLOCK_REFERENCE * 1000000) / dsor > ios->clock)
			dsor++;
	}

	mmc_reg_out(&mmc_base->sysctl, (ICE_MASK | DTO_MASK | CEN_MASK),
				(ICE_STOP | DTO_15THDTO | CEN_DISABLE));

	mmc_reg_out(&mmc_base->sysctl, ICE_MASK | CLKD_MASK,
				(dsor << CLKD_OFFSET) | ICE_OSCILLATE);

	start = get_time_ns();
	while ((readl(&mmc_base->sysctl) & ICS_MASK) == ICS_NOTREADY) {
		if (is_timeout(start, SECOND)) {
			dev_dbg(hsmmc->dev, "timedout waiting for ics!\n");
			return;
		}
	}
	writel(readl(&mmc_base->sysctl) | CEN_ENABLE, &mmc_base->sysctl);
}
