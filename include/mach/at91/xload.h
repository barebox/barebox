/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_AT91_XLOAD_H
#define __MACH_AT91_XLOAD_H

#include <linux/compiler.h>
#include <pbl/bio.h>
#include <pbl/mci.h>

void __noreturn sama5d2_start_image(u32 r4);
void __noreturn sama5d3_atmci_start_image(u32 r4, unsigned int clock,
					  unsigned int slot);

int at91_sdhci_bio_init(struct pbl_bio *bio, void __iomem *base);
int at91_mci_bio_init(struct pbl_bio *bio, void __iomem *base,
		      unsigned int clock, unsigned int slot,
		      enum pbl_mci_capacity capacity);

void __noreturn sam9263_atmci_start_image(u32 mmc_id, unsigned int clock,
					  bool slot_b);


#endif /* __MACH_AT91_XLOAD_H */
