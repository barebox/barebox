#ifndef _MACH_GENERIC_H
#define _MACH_GENERIC_H

/* I2C controller revisions */
#define OMAP_I2C_REV_2			0x20

/* I2C controller revisions present on specific hardware */
#define OMAP_I2C_REV_ON_2430		0x36
#define OMAP_I2C_REV_ON_3430		0x3C
#define OMAP_I2C_REV_ON_4430		0x40

#ifdef CONFIG_ARCH_OMAP
#define cpu_is_omap2430()	(1)
#else
#define cpu_is_omap2430()	(0)
#endif

#ifdef CONFIG_ARCH_OMAP3
#define cpu_is_omap34xx()	(1)
#else
#define cpu_is_omap34xx()	(0)
#endif

#ifdef CONFIG_ARCH_OMAP4
#define cpu_is_omap4xxx()	(1)
#else
#define cpu_is_omap4xxx()	(0)
#endif

enum omap_boot_src {
	OMAP_BOOTSRC_UNKNOWN,
	OMAP_BOOTSRC_MMC1,
	OMAP_BOOTSRC_NAND,
	OMAP_BOOTSRC_SPI1,
	OMAP_BOOTSRC_USB1,
};

enum omap_boot_src omap_bootsrc(void);
enum omap_boot_src am33xx_bootsrc(void);
enum omap_boot_src omap3_bootsrc(void);
enum omap_boot_src omap4_bootsrc(void);

#endif
