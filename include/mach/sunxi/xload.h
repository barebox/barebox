/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_XLOAD_H
#define __MACH_XLOAD_H

#include <linux/compiler.h>
#include <pbl/bio.h>

int sunxi_mmc_bio_init(struct pbl_bio *bio, void __iomem *base,
		       unsigned int clock, unsigned int slot);

#endif
