#ifndef __MACH_LAYERSCAPE_H
#define __MACH_LAYERSCAPE_H

#define LS1046A_DDR_SDRAM_BASE	0x80000000
#define LS1046A_DDR_FREQ	2100000000

enum bootsource ls1046_bootsource_get(void);

#endif /* __MACH_LAYERSCAPE_H */
