#ifndef _MACH_XLOAD_H
#define _MACH_XLOAD_H

enum omap_boot_src {
	OMAP_BOOTSRC_UNKNOWN,
	OMAP_BOOTSRC_MMC1,
	OMAP_BOOTSRC_NAND,
};

enum omap_boot_src omap3_bootsrc(void);
enum omap_boot_src omap4_bootsrc(void);

#endif /* _MACH_XLOAD_H */
