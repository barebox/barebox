/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_OMAP_XLOAD_H
#define __MACH_OMAP_XLOAD_H

#include <linux/compiler.h>
#include <pbl/bio.h>

void __noreturn am33xx_hsmmc_start_image(void);

int omap_hsmmc_bio_init(struct pbl_bio *bio, void __iomem *base,
			unsigned reg_ofs);

#endif /* __MACH_OMAP_XLOAD_H */
