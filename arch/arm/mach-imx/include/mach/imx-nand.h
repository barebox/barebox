#ifndef __ASM_ARCH_NAND_H
#define __ASM_ARCH_NAND_H

#include <linux/mtd/mtd.h>

void imx_nand_load_image(void *dest, int size);
void imx_nand_set_layout(int writesize, int datawidth);

struct imx_nand_platform_data {
	int width;
	int hw_ecc:1;
	int flash_bbt:1;
};
#endif /* __ASM_ARCH_NAND_H */

