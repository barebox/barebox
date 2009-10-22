#ifndef __ASM_ARCH_NAND_H
#define __ASM_ARCH_NAND_H

#include <linux/mtd/mtd.h>

void imx_nand_load_image(void *dest, int size, int pagesize, int blocksize);

struct imx_nand_platform_data {
	int width;
	int hw_ecc;
	int is2k;
	};
#endif /* __ASM_ARCH_NAND_H */

