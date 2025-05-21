// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <mach/at91/xload.h>
#include <mci.h>

#include "atmel-mci-regs.h"

#define SUPPORT_MAX_BLOCKS		16U

static int pbl_atmci_common_request(struct pbl_mci *mci,
				    struct mci_cmd *cmd,
				    struct mci_data *data)
{
	return atmci_common_request(mci->priv, cmd, data);
}

static struct atmel_mci atmci_host;
static struct pbl_mci mci;

int at91_mci_bio_init(struct pbl_bio *bio, void __iomem *base,
		      unsigned int clock, unsigned int slot,
		      enum pbl_mci_capacity capacity)
{
	struct atmel_mci *host = &atmci_host;
	struct mci_ios ios = { .bus_width = MMC_BUS_WIDTH_4, .clock = 25000000 };

	mci.priv = host;
	mci.send_cmd = pbl_atmci_common_request;

	/*
	 * PBL will get MCI controller in disabled state,
	 * so we need to reconfigure it.
	 */
	host->regs = base;

	atmci_get_cap(host);

	host->bus_hz = clock;

	host->slot_b = slot;
	if (host->slot_b)
		host->sdc_reg = ATMCI_SDCSEL_SLOT_B;
	else
		host->sdc_reg = ATMCI_SDCSEL_SLOT_A;

	atmci_writel(host, ATMCI_CR, ATMCI_CR_PWSDIS);
	atmci_writel(host, ATMCI_DTOR, 0x7f);

	atmci_common_set_ios(host, &ios);

	mci.priv = host;
	mci.send_cmd = pbl_atmci_common_request;
	mci.max_blocks_per_read = SUPPORT_MAX_BLOCKS;
	mci.capacity = capacity;

	return pbl_mci_bio_init(&mci, bio);
}
