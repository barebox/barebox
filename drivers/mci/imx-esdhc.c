/*
 * Copyright 2007,2010 Freescale Semiconductor, Inc
 * Andy Fleming
 *
 * Based vaguely on the pxa mmc code:
 * (C) Copyright 2003
 * Kyle Harris, Nexus Technologies, Inc. kharris@nexus-tech.net
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <mci.h>
#include <clock.h>
#include <asm/io.h>
#include <asm/mmu.h>
#include <mach/clock.h>

#include "imx-esdhc.h"

struct fsl_esdhc {
	u32	dsaddr;
	u32	blkattr;
	u32	cmdarg;
	u32	xfertyp;
	u32	cmdrsp0;
	u32	cmdrsp1;
	u32	cmdrsp2;
	u32	cmdrsp3;
	u32	datport;
	u32	prsstat;
	u32	proctl;
	u32	sysctl;
	u32	irqstat;
	u32	irqstaten;
	u32	irqsigen;
	u32	autoc12err;
	u32	hostcapblt;
	u32	wml;
	char	reserved1[8];
	u32	fevt;
	char	reserved2[168];
	u32	hostver;
	char	reserved3[780];
	u32	scr;
};

struct fsl_esdhc_host {
	struct mci_host		mci;
	struct fsl_esdhc	*regs;
	u32			no_snoop;
	unsigned long		cur_clock;
	struct device_d		*dev;
};

#define to_fsl_esdhc(mci)	container_of(mci, struct fsl_esdhc_host, mci)

/* Return the XFERTYP flags for a given command and data packet */
u32 esdhc_xfertyp(struct mci_cmd *cmd, struct mci_data *data)
{
	u32 xfertyp = 0;

	if (data) {
		xfertyp |= XFERTYP_DPSEL;
#ifndef CONFIG_MCI_IMX_ESDHC_PIO
		xfertyp |= XFERTYP_DMAEN;
#endif
		if (data->blocks > 1) {
			xfertyp |= XFERTYP_MSBSEL;
			xfertyp |= XFERTYP_BCEN;
		}

		if (data->flags & MMC_DATA_READ)
			xfertyp |= XFERTYP_DTDSEL;
	}

	if (cmd->resp_type & MMC_RSP_CRC)
		xfertyp |= XFERTYP_CCCEN;
	if (cmd->resp_type & MMC_RSP_OPCODE)
		xfertyp |= XFERTYP_CICEN;
	if (cmd->resp_type & MMC_RSP_136)
		xfertyp |= XFERTYP_RSPTYP_136;
	else if (cmd->resp_type & MMC_RSP_BUSY)
		xfertyp |= XFERTYP_RSPTYP_48_BUSY;
	else if (cmd->resp_type & MMC_RSP_PRESENT)
		xfertyp |= XFERTYP_RSPTYP_48;

	return XFERTYP_CMD(cmd->cmdidx) | xfertyp;
}

#ifdef CONFIG_MCI_IMX_ESDHC_PIO
/*
 * PIO Read/Write Mode reduce the performace as DMA is not used in this mode.
 */
static void
esdhc_pio_read_write(struct mci_host *mci, struct mci_data *data)
{
	struct fsl_esdhc_host *host = to_fsl_esdhc(mci);
	struct fsl_esdhc *regs = host->regs;
	u32 blocks;
	char *buffer;
	u32 databuf;
	u32 size;
	u32 irqstat;
	u32 timeout;

	if (data->flags & MMC_DATA_READ) {
		blocks = data->blocks;
		buffer = data->dest;
		while (blocks) {
			timeout = PIO_TIMEOUT;
			size = data->blocksize;
			irqstat = esdhc_read32(&regs->irqstat);
			while (!(esdhc_read32(&regs->prsstat) & PRSSTAT_BREN)
				&& --timeout);
			if (timeout <= 0) {
				printf("\nData Read Failed in PIO Mode.");
				return;
			}
			while (size && (!(irqstat & IRQSTAT_TC))) {
				udelay(100); /* Wait before last byte transfer complete */
				irqstat = esdhc_read32(&regs->irqstat);
				databuf = esdhc_read32(&regs->datport);
				*((u32 *)buffer) = databuf;
				buffer += 4;
				size -= 4;
			}
			blocks--;
		}
	} else {
		blocks = data->blocks;
		buffer = (char *)data->src;
		while (blocks) {
			timeout = PIO_TIMEOUT;
			size = data->blocksize;
			irqstat = esdhc_read32(&regs->irqstat);
			while (!(esdhc_read32(&regs->prsstat) & PRSSTAT_BWEN)
				&& --timeout);
			if (timeout <= 0) {
				printf("\nData Write Failed in PIO Mode.");
				return;
			}
			while (size && (!(irqstat & IRQSTAT_TC))) {
				udelay(100); /* Wait before last byte transfer complete */
				databuf = *((u32 *)buffer);
				buffer += 4;
				size -= 4;
				irqstat = esdhc_read32(&regs->irqstat);
				esdhc_write32(&regs->datport, databuf);
			}
			blocks--;
		}
	}
}
#endif

static int esdhc_setup_data(struct mci_host *mci, struct mci_data *data)
{
	struct fsl_esdhc_host *host = to_fsl_esdhc(mci);
	struct fsl_esdhc *regs = host->regs;
#ifndef CONFIG_MCI_IMX_ESDHC_PIO
	u32 wml_value;

	wml_value = data->blocksize/4;

	if (data->flags & MMC_DATA_READ) {
		if (wml_value > 0x10)
			wml_value = 0x10;

		esdhc_clrsetbits32(&regs->wml, WML_RD_WML_MASK, wml_value);
		esdhc_write32(&regs->dsaddr, (u32)data->dest);
	} else {
		if (wml_value > 0x80)
			wml_value = 0x80;
		if ((esdhc_read32(&regs->prsstat) & PRSSTAT_WPSPL) == 0) {
			printf("\nThe SD card is locked. Can not write to a locked card.\n\n");
			return -ETIMEDOUT;
		}

		esdhc_clrsetbits32(&regs->wml, WML_WR_WML_MASK,
					wml_value << 16);
		esdhc_write32(&regs->dsaddr, (u32)data->src);
	}
#else	/* CONFIG_MCI_IMX_ESDHC_PIO */
	if (!(data->flags & MMC_DATA_READ)) {
		if ((esdhc_read32(&regs->prsstat) & PRSSTAT_WPSPL) == 0) {
			printf("\nThe SD card is locked. "
				"Can not write to a locked card.\n\n");
			return -ETIMEDOUT;
		}
		esdhc_write32(&regs->dsaddr, (u32)data->src);
	} else
		esdhc_write32(&regs->dsaddr, (u32)data->dest);
#endif	/* CONFIG_MCI_IMX_ESDHC_PIO */

	esdhc_write32(&regs->blkattr, data->blocks << 16 | data->blocksize);

	return 0;
}


/*
 * Sends a command out on the bus.  Takes the mci pointer,
 * a command pointer, and an optional data pointer.
 */
static int
esdhc_send_cmd(struct mci_host *mci, struct mci_cmd *cmd, struct mci_data *data)
{
	u32	xfertyp;
	u32	irqstat;
	struct fsl_esdhc_host *host = to_fsl_esdhc(mci);
	struct fsl_esdhc *regs = host->regs;

	esdhc_write32(&regs->irqstat, -1);

	/* Wait for the bus to be idle */
	while ((esdhc_read32(&regs->prsstat) & PRSSTAT_CICHB) ||
			(esdhc_read32(&regs->prsstat) & PRSSTAT_CIDHB))
		;

	while (esdhc_read32(&regs->prsstat) & PRSSTAT_DLA)
		;

	/* Wait at least 8 SD clock cycles before the next command */
	/*
	 * Note: This is way more than 8 cycles, but 1ms seems to
	 * resolve timing issues with some cards
	 */
	udelay(1000);

	/* Set up for a data transfer if we have one */
	if (data) {
		int err;

		err = esdhc_setup_data(mci, data);
		if(err)
			return err;
		if (data->flags & MMC_DATA_WRITE) {
			dma_flush_range((unsigned long)data->src,
				(unsigned long)(data->src + 512));
		} else
			dma_clean_range((unsigned long)data->src,
				(unsigned long)(data->src + 512));

	}

	/* Figure out the transfer arguments */
	xfertyp = esdhc_xfertyp(cmd, data);

	/* Send the command */
	esdhc_write32(&regs->cmdarg, cmd->cmdarg);
	esdhc_write32(&regs->xfertyp, xfertyp);

	/* Wait for the command to complete */
	while (!(esdhc_read32(&regs->irqstat) & IRQSTAT_CC))
		;

	irqstat = esdhc_read32(&regs->irqstat);
	esdhc_write32(&regs->irqstat, irqstat);

	if (irqstat & CMD_ERR)
		return -EIO;

	if (irqstat & IRQSTAT_CTOE)
		return -ETIMEDOUT;

	/* Copy the response to the response buffer */
	if (cmd->resp_type & MMC_RSP_136) {
		u32 cmdrsp3, cmdrsp2, cmdrsp1, cmdrsp0;

		cmdrsp3 = esdhc_read32(&regs->cmdrsp3);
		cmdrsp2 = esdhc_read32(&regs->cmdrsp2);
		cmdrsp1 = esdhc_read32(&regs->cmdrsp1);
		cmdrsp0 = esdhc_read32(&regs->cmdrsp0);
		cmd->response[0] = (cmdrsp3 << 8) | (cmdrsp2 >> 24);
		cmd->response[1] = (cmdrsp2 << 8) | (cmdrsp1 >> 24);
		cmd->response[2] = (cmdrsp1 << 8) | (cmdrsp0 >> 24);
		cmd->response[3] = (cmdrsp0 << 8);
	} else
		cmd->response[0] = esdhc_read32(&regs->cmdrsp0);

	/* Wait until all of the blocks are transferred */
	if (data) {
#ifdef CONFIG_MCI_IMX_ESDHC_PIO
		esdhc_pio_read_write(mci, data);
#else
		do {
			irqstat = esdhc_read32(&regs->irqstat);

			if (irqstat & DATA_ERR)
				return -EIO;

			if (irqstat & IRQSTAT_DTOE)
				return -ETIMEDOUT;
		} while (!(irqstat & IRQSTAT_TC) &&
				(esdhc_read32(&regs->prsstat) & PRSSTAT_DLA));

		if (data->flags & MMC_DATA_READ) {
			dma_inv_range((unsigned long)data->dest,
					(unsigned long)(data->dest + 512));
		}
#endif
	}

	esdhc_write32(&regs->irqstat, -1);

	return 0;
}

void set_sysctl(struct mci_host *mci, u32 clock)
{
	int div, pre_div;
	struct fsl_esdhc_host *host = to_fsl_esdhc(mci);
	struct fsl_esdhc *regs = host->regs;
	int sdhc_clk = imx_get_mmcclk();
	u32 clk;

	if (clock < mci->f_min)
		clock = mci->f_min;

	pre_div = 0;

	for (pre_div = 1; pre_div < 256; pre_div <<= 1) {
		if (sdhc_clk / pre_div < clock * 16)
			break;
	};

	div = sdhc_clk / pre_div / clock;

	if (sdhc_clk / pre_div / div > clock)
		div++;

	host->cur_clock = sdhc_clk / pre_div / div;

	div -= 1;
	pre_div >>= 1;

	dev_dbg(host->dev, "set clock: wanted: %d got: %ld\n", clock, host->cur_clock);
	dev_dbg(host->dev, "pre_div: %d div: %d\n", pre_div, div);

	clk = (pre_div << 8) | (div << 4);

	esdhc_clrbits32(&regs->sysctl, SYSCTL_CKEN);

	esdhc_clrsetbits32(&regs->sysctl, SYSCTL_CLOCK_MASK, clk);

	udelay(10000);

	clk = SYSCTL_PEREN | SYSCTL_CKEN;

	esdhc_setbits32(&regs->sysctl, clk);
}

static void esdhc_set_ios(struct mci_host *mci, struct device_d *dev,
		unsigned bus_width, unsigned clock)
{
	struct fsl_esdhc_host *host = to_fsl_esdhc(mci);
	struct fsl_esdhc *regs = host->regs;

	/* Set the clock speed */
	set_sysctl(mci, clock);

	/* Set the bus width */
	esdhc_clrbits32(&regs->proctl, PROCTL_DTW_4 | PROCTL_DTW_8);

	if (bus_width == 4)
		esdhc_setbits32(&regs->proctl, PROCTL_DTW_4);
	else if (bus_width == 8)
		esdhc_setbits32(&regs->proctl, PROCTL_DTW_8);

}

static int esdhc_init(struct mci_host *mci, struct device_d *dev)
{
	struct fsl_esdhc_host *host = to_fsl_esdhc(mci);
	struct fsl_esdhc *regs = host->regs;
	int timeout = 1000;
	int ret = 0;

	/* Enable cache snooping */
	if (host && !host->no_snoop)
		esdhc_write32(&regs->scr, 0x00000040);

	/* Reset the entire host controller */
	esdhc_write32(&regs->sysctl, SYSCTL_RSTA);

	/* Wait until the controller is available */
	while ((esdhc_read32(&regs->sysctl) & SYSCTL_RSTA) && --timeout)
		udelay(1000);

	esdhc_write32(&regs->sysctl, SYSCTL_HCKEN | SYSCTL_IPGEN);

	/* Set the initial clock speed */
	set_sysctl(mci, 400000);

	/* Disable the BRR and BWR bits in IRQSTAT */
	esdhc_clrbits32(&regs->irqstaten, IRQSTATEN_BRR | IRQSTATEN_BWR);

	/* Put the PROCTL reg back to the default */
	esdhc_write32(&regs->proctl, PROCTL_INIT);

	/* Set timout to the maximum value */
	esdhc_clrsetbits32(&regs->sysctl, SYSCTL_TIMEOUT_MASK, 14 << 16);

	return ret;
}

static int esdhc_reset(struct fsl_esdhc *regs)
{
	uint64_t start;

	/* reset the controller */
	esdhc_write32(&regs->sysctl, SYSCTL_RSTA);

	start = get_time_ns();
	/* hardware clears the bit when it is done */
	while (1) {
		if (!(esdhc_read32(&regs->sysctl) & SYSCTL_RSTA))
			break;
		if (is_timeout(start, 100 * MSECOND)) {
			printf("MMC/SD: Reset never completed.\n");
			return -EIO;
		}
	}

	return 0;
}

static int fsl_esdhc_probe(struct device_d *dev)
{
	struct fsl_esdhc_host *host;
	struct mci_host *mci;
	u32 caps;
	int ret;

	host = xzalloc(sizeof(*host));
	mci = &host->mci;

	host->dev = dev;
	host->regs = (struct fsl_esdhc *)dev->map_base;

	/* First reset the eSDHC controller */
	ret = esdhc_reset(host->regs);
	if (ret) {
		free(host);
		return ret;
	}

	caps = esdhc_read32(&host->regs->hostcapblt);

	if (caps & ESDHC_HOSTCAPBLT_VS18)
		mci->voltages |= MMC_VDD_165_195;
	if (caps & ESDHC_HOSTCAPBLT_VS30)
		mci->voltages |= MMC_VDD_29_30 | MMC_VDD_30_31;
	if (caps & ESDHC_HOSTCAPBLT_VS33)
		mci->voltages |= MMC_VDD_32_33 | MMC_VDD_33_34;

	mci->host_caps = MMC_MODE_4BIT | MMC_MODE_8BIT;

	if (caps & ESDHC_HOSTCAPBLT_HSS)
		mci->host_caps |= MMC_MODE_HS_52MHz | MMC_MODE_HS;

	host->mci.send_cmd = esdhc_send_cmd;
	host->mci.set_ios = esdhc_set_ios;
	host->mci.init = esdhc_init;
	host->mci.host_caps = MMC_MODE_4BIT;

	host->mci.voltages = MMC_VDD_32_33 | MMC_VDD_33_34;

	host->mci.f_min = imx_get_mmcclk() >> 12;
	if (host->mci.f_min < 200000)
		host->mci.f_min = 200000;
	host->mci.f_max = imx_get_mmcclk();

	mci_register(&host->mci);

	return 0;
}

static struct driver_d fsl_esdhc_driver = {
        .name  = "imx-esdhc",
        .probe = fsl_esdhc_probe,
};

static int fsl_esdhc_init_driver(void)
{
        register_driver(&fsl_esdhc_driver);
        return 0;
}

device_initcall(fsl_esdhc_init_driver);

