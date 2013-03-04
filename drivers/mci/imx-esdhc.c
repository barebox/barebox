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
 */
#include <config.h>
#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <mci.h>
#include <clock.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <asm/mmu.h>
#include <mach/generic.h>
#include <mach/esdhc.h>
#include <gpio.h>

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
	u32	mixctrl;
	char	reserved1[4];
	u32	fevt;
	char	reserved2[168];
	u32	hostver;
	char	reserved3[780];
	u32	scr;
};

struct fsl_esdhc_host {
	struct mci_host		mci;
	struct fsl_esdhc __iomem	*regs;
	u32			no_snoop;
	unsigned long		cur_clock;
	struct device_d		*dev;
	struct clk		*clk;
};

#define to_fsl_esdhc(mci)	container_of(mci, struct fsl_esdhc_host, mci)

#define  SDHCI_CMD_ABORTCMD (0xC0 << 16)

/* Return the XFERTYP flags for a given command and data packet */
static u32 esdhc_xfertyp(struct mci_cmd *cmd, struct mci_data *data)
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
	if ((cpu_is_mx51() || cpu_is_mx53()) &&
			cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION)
		xfertyp |= SDHCI_CMD_ABORTCMD;

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
	struct fsl_esdhc __iomem *regs = host->regs;
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
	u32	xfertyp, mixctrl;
	u32	irqstat;
	struct fsl_esdhc_host *host = to_fsl_esdhc(mci);
	struct fsl_esdhc __iomem *regs = host->regs;
	int ret;

	esdhc_write32(&regs->irqstat, -1);

	/* Wait at least 8 SD clock cycles before the next command */
	udelay(1);

	/* Set up for a data transfer if we have one */
	if (data) {
		int err;

		err = esdhc_setup_data(mci, data);
		if(err)
			return err;
		if (data->flags & MMC_DATA_WRITE) {
			dma_flush_range((unsigned long)data->src,
				(unsigned long)(data->src + data->blocks * 512));
		} else
			dma_clean_range((unsigned long)data->src,
				(unsigned long)(data->src + data->blocks * 512));

	}

	/* Figure out the transfer arguments */
	xfertyp = esdhc_xfertyp(cmd, data);

	/* Send the command */
	esdhc_write32(&regs->cmdarg, cmd->cmdarg);

	if (cpu_is_mx6()) {
		/* write lower-half of xfertyp to mixctrl */
		mixctrl = xfertyp & 0xFFFF;
		/* Keep the bits 22-25 of the register as is */
		mixctrl |= (esdhc_read32(&regs->mixctrl) & (0xF << 22));
		esdhc_write32(&regs->mixctrl, mixctrl);
	}

	esdhc_write32(&regs->xfertyp, xfertyp);

	/* Wait for the command to complete */
	ret = wait_on_timeout(100 * MSECOND,
			esdhc_read32(&regs->irqstat) & IRQSTAT_CC);
	if (ret) {
		dev_dbg(host->dev, "timeout 1\n");
		return -ETIMEDOUT;
	}

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
					(unsigned long)(data->dest + data->blocks * 512));
		}
#endif
	}

	esdhc_write32(&regs->irqstat, -1);

	/* Wait for the bus to be idle */
	ret = wait_on_timeout(SECOND,
			!(esdhc_read32(&regs->prsstat) &
				(PRSSTAT_CICHB | PRSSTAT_CIDHB)));
	if (ret) {
		dev_err(host->dev, "timeout 2\n");
		return -ETIMEDOUT;
	}

	ret = wait_on_timeout(100 * MSECOND,
			!(esdhc_read32(&regs->prsstat) & PRSSTAT_DLA));
	if (ret) {
		dev_err(host->dev, "timeout 3\n");
		return -ETIMEDOUT;
	}

	return 0;
}

static void set_sysctl(struct mci_host *mci, u32 clock)
{
	int div, pre_div;
	struct fsl_esdhc_host *host = to_fsl_esdhc(mci);
	struct fsl_esdhc __iomem *regs = host->regs;
	int sdhc_clk = clk_get_rate(host->clk);
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

static void esdhc_set_ios(struct mci_host *mci, struct mci_ios *ios)
{
	struct fsl_esdhc_host *host = to_fsl_esdhc(mci);
	struct fsl_esdhc __iomem *regs = host->regs;

	/* Set the clock speed */
	set_sysctl(mci, ios->clock);

	/* Set the bus width */
	esdhc_clrbits32(&regs->proctl, PROCTL_DTW_4 | PROCTL_DTW_8);

	switch (ios->bus_width) {
	case MMC_BUS_WIDTH_4:
		esdhc_setbits32(&regs->proctl, PROCTL_DTW_4);
		break;
	case MMC_BUS_WIDTH_8:
		esdhc_setbits32(&regs->proctl, PROCTL_DTW_8);
		break;
	case MMC_BUS_WIDTH_1:
		break;
	default:
		return;
	}

}

static int esdhc_card_present(struct mci_host *mci)
{
	struct fsl_esdhc_host *host = to_fsl_esdhc(mci);
	struct fsl_esdhc __iomem *regs = host->regs;
	struct esdhc_platform_data *pdata = host->dev->platform_data;
	int ret;

	if (!pdata)
		return 1;

	switch (pdata->cd_type) {
	case ESDHC_CD_NONE:
	case ESDHC_CD_PERMANENT:
		return 1;
	case ESDHC_CD_CONTROLLER:
		return !(esdhc_read32(&regs->prsstat) & PRSSTAT_WPSPL);
	case ESDHC_CD_GPIO:
		ret = gpio_direction_input(pdata->cd_gpio);
		if (ret)
			return ret;
		return gpio_get_value(pdata->cd_gpio) ? 0 : 1;
	}

	return 0;
}

static int esdhc_init(struct mci_host *mci, struct device_d *dev)
{
	struct fsl_esdhc_host *host = to_fsl_esdhc(mci);
	struct fsl_esdhc __iomem *regs = host->regs;
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

static int esdhc_reset(struct fsl_esdhc __iomem *regs)
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
	unsigned long rate;
	struct esdhc_platform_data *pdata = dev->platform_data;

	host = xzalloc(sizeof(*host));
	mci = &host->mci;

	host->clk = clk_get(dev, NULL);
	if (IS_ERR(host->clk))
		return PTR_ERR(host->clk);

	host->dev = dev;
	host->regs = dev_request_mem_region(dev, 0);

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

	if (pdata && pdata->caps)
		mci->host_caps = pdata->caps;
	else
		mci->host_caps = MMC_MODE_4BIT;

	if (pdata && pdata->devname)
		mci->devname = pdata->devname;

	if (caps & ESDHC_HOSTCAPBLT_HSS)
		mci->host_caps |= MMC_MODE_HS_52MHz | MMC_MODE_HS;

	host->mci.send_cmd = esdhc_send_cmd;
	host->mci.set_ios = esdhc_set_ios;
	host->mci.init = esdhc_init;
	host->mci.card_present = esdhc_card_present;
	host->mci.hw_dev = dev;

	rate = clk_get_rate(host->clk);
	host->mci.f_min = rate >> 12;
	if (host->mci.f_min < 200000)
		host->mci.f_min = 200000;
	host->mci.f_max = rate;

	mci_register(&host->mci);

	return 0;
}

static __maybe_unused struct of_device_id fsl_esdhc_compatible[] = {
	{
		.compatible = "fsl,imx51-esdhc",
	}, {
		.compatible = "fsl,imx53-esdhc",
	}, {
		.compatible = "fsl,imx6q-usdhc",
	}, {
		/* sentinel */
	}
};

static struct driver_d fsl_esdhc_driver = {
	.name  = "imx-esdhc",
	.probe = fsl_esdhc_probe,
	.of_compatible = DRV_OF_COMPAT(fsl_esdhc_compatible),
};
device_platform_driver(fsl_esdhc_driver);
