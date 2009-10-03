#ifndef __ASM_ARCH_NAND_H
#define __ASM_ARCH_NAND_H

#include <linux/mtd/mtd.h>

void atmel_nand_load_image(void *dest, int size, int pagesize, int blocksize);

 /* NAND / SmartMedia */
struct atmel_nand_data {
	void __iomem	*ecc_base;
	u8		enable_pin;	/* chip enable */
	u8		det_pin;	/* card detect */
	u8		rdy_pin;	/* ready/busy */
	u8		ale;		/* address line number connected to ALE */
	u8		cle;		/* address line number connected to CLE */
	u8		bus_width_16;	/* buswidth is 16 bit */
	u8		ecc_mode;	/* NAND_ECC_* */
};

#endif /* __ASM_ARCH_NAND_H */

