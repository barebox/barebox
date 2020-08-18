#ifndef __MACH_XLOAD_H
#define __MACH_XLOAD_H

#include <linux/compiler.h>
#include <pbl.h>

void __noreturn sama5d2_sdhci_start_image(u32 r4);

int at91_sdhci_bio_init(struct pbl_bio *bio, void __iomem *base);

#endif /* __MACH_XLOAD_H */
