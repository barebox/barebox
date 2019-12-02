// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <io.h>
#include <mci.h>
#include <pbl.h>

#include "sdhci.h"
#include "imx-esdhc.h"

static u32 esdhc_op_read32_le(struct sdhci *sdhci, int reg)
{
	struct fsl_esdhc_host *host = sdhci_to_esdhc(sdhci);

	return readl(host->regs + reg);
}

static u32 esdhc_op_read32_be(struct sdhci *sdhci, int reg)
{
	struct fsl_esdhc_host *host = sdhci_to_esdhc(sdhci);

	return in_be32(host->regs + reg);
}

static void esdhc_op_write32_le(struct sdhci *sdhci, int reg, u32 val)
{
	struct fsl_esdhc_host *host = sdhci_to_esdhc(sdhci);

	writel(val, host->regs + reg);
}

static void esdhc_op_write32_be(struct sdhci *sdhci, int reg, u32 val)
{
	struct fsl_esdhc_host *host = sdhci_to_esdhc(sdhci);

	out_be32(host->regs + reg, val);
}

void esdhc_populate_sdhci(struct fsl_esdhc_host *host)
{
	if (host->socdata->flags & ESDHC_FLAG_BIGENDIAN) {
		host->sdhci.read32 = esdhc_op_read32_be;
		host->sdhci.write32 = esdhc_op_write32_be;
	} else {
		host->sdhci.read32 = esdhc_op_read32_le;
		host->sdhci.write32 = esdhc_op_write32_le;
	}
}

static bool esdhc_use_pio_mode(void)
{
	return IN_PBL || IS_ENABLED(CONFIG_MCI_IMX_ESDHC_PIO);
}

int esdhc_setup_data(struct fsl_esdhc_host *host, struct mci_data *data,
		     struct fsl_esdhc_dma_transfer *tr)
{
	u32 wml_value;
	void *ptr;

	if (!esdhc_use_pio_mode()) {
		wml_value = data->blocksize/4;

		if (data->flags & MMC_DATA_READ) {
			if (wml_value > 0x10)
				wml_value = 0x10;

			esdhc_clrsetbits32(host, IMX_SDHCI_WML, WML_RD_WML_MASK, wml_value);
		} else {
			if (wml_value > 0x80)
				wml_value = 0x80;

			esdhc_clrsetbits32(host, IMX_SDHCI_WML, WML_WR_WML_MASK,
						wml_value << 16);
		}

		tr->size = data->blocks * data->blocksize;

		if (data->flags & MMC_DATA_WRITE) {
			ptr = (void *)data->src;
			tr->dir = DMA_TO_DEVICE;
		} else {
			ptr = data->dest;
			tr->dir = DMA_FROM_DEVICE;
		}

		tr->dma = dma_map_single(host->dev, ptr, tr->size, tr->dir);
		if (dma_mapping_error(host->dev, tr->dma))
			return -EFAULT;


		sdhci_write32(&host->sdhci, SDHCI_DMA_ADDRESS, tr->dma);
	}

	sdhci_write32(&host->sdhci, SDHCI_BLOCK_SIZE__BLOCK_COUNT, data->blocks << 16 | data->blocksize);

	return 0;
}

int esdhc_do_data(struct fsl_esdhc_host *host, struct mci_data *data,
		  struct fsl_esdhc_dma_transfer *tr)
{
	u32 irqstat;

	if (esdhc_use_pio_mode())
		return sdhci_transfer_data(&host->sdhci, data);

	do {
		irqstat = sdhci_read32(&host->sdhci, SDHCI_INT_STATUS);

		if (irqstat & DATA_ERR)
			return -EIO;

		if (irqstat & SDHCI_INT_DATA_TIMEOUT)
			return -ETIMEDOUT;
	} while (!(irqstat & SDHCI_INT_XFER_COMPLETE) &&
		(sdhci_read32(&host->sdhci, SDHCI_PRESENT_STATE) & SDHCI_DATA_LINE_ACTIVE));

	dma_unmap_single(host->dev, tr->dma, tr->size, tr->dir);

	return 0;
}
