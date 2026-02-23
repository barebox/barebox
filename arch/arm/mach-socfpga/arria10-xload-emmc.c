// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <init.h>
#include <linux/sizes.h>
#include <mach/socfpga/generic.h>
#include <mach/socfpga/arria10-regs.h>
#include <mach/socfpga/arria10-system-manager.h>
#include <mach/socfpga/arria10-xload.h>
#include <disks.h>
#include <mci.h>
#include <pbl/mci.h>
#include <pbl/bio.h>
#include "../../../drivers/mci/dw_mmc.h"

static struct pbl_bio bio;

int arria10_read_blocks(void *dst, int blocknum, size_t len)
{
	int blocks;

	blocks = len / SECTOR_SIZE;

	return pbl_bio_read(&bio, blocknum, dst, blocks);
}

void arria10_init_mmc(void)
{
	void __iomem *base = IOMEM(ARRIA10_SDMMC_ADDR);

	writel(ARRIA10_SYSMGR_SDMMC_DRVSEL(3) |
	       ARRIA10_SYSMGR_SDMMC_SMPLSEL(2),
	       ARRIA10_SYSMGR_SDMMC);

	writel(DWMCI_CTYPE_1BIT, base + DWMCI_CTYPE);

	dw_mmc_pbl_bio_init(&bio, base);
}
