// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2008 Texas Instruments (http://www.ti.com/, Sukumar Ghorai <s-ghorai@ti.com>)

#include <pbl/mci.h>
#include <pbl/bio.h>
#include <mci.h>
#include <debug_ll.h>
#include <mach/omap/xload.h>

#include "omap_hsmmc.h"

static int pbl_omap_hsmmc_send_cmd(struct pbl_mci *mci,
				   struct mci_cmd *cmd,
				   struct mci_data *data)
{
	return omap_hsmmc_send_cmd(mci->priv, cmd, data);
}

static struct omap_hsmmc hsmmc;
static struct pbl_mci mci;

int omap_hsmmc_bio_init(struct pbl_bio *bio, void __iomem *iobase,
			unsigned reg_ofs)
{
	hsmmc.iobase = iobase;
	hsmmc.base = iobase + reg_ofs;

	mci.priv = &hsmmc;
	mci.send_cmd = pbl_omap_hsmmc_send_cmd;

	return pbl_mci_bio_init(&mci, bio);
}
