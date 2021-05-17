#ifndef __MACH_XLOAD_H
#define __MACH_XLOAD_H

#include <linux/compiler.h>
#include <pbl.h>

void __noreturn sama5d2_sdhci_start_image(u32 r4);
void __noreturn sama5d3_atmci_start_image(u32 r4, unsigned int clock,
					  unsigned int slot);

int at91_sdhci_bio_init(struct pbl_bio *bio, void __iomem *base);
int at91_mci_bio_init(struct pbl_bio *bio, void __iomem *base,
		      unsigned int clock, unsigned int slot);

#endif /* __MACH_XLOAD_H */
