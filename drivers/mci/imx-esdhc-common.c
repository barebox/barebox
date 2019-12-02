// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <io.h>
#include <mci.h>

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
