#ifndef __OMAP_MCSPI_H
#define  __OMAP_MCSPI_H

#define OMAP3_MCSPI1_BASE	0x48098000
#define OMAP3_MCSPI2_BASE	0x4809A000
#define OMAP3_MCSPI3_BASE	0x480B8000
#define OMAP3_MCSPI4_BASE	0x480BA000

int mcspi_devices_init(void);

#endif /* __OMAP_MCSPI_H */
