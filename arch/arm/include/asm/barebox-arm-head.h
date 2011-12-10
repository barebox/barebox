#ifndef __ASM_ARM_HEAD_H
#define __ASM_ARM_HEAD_H

static inline void barebox_arm_head(void)
{
	__asm__ __volatile__ (
#ifdef CONFIG_THUMB2_BAREBOX
		".arm\n"
		"adr r9, 1f + 1\n"
		"bx r9\n"
		".thumb\n"
		"1:\n"
		"bl reset\n"
		".rept 10\n"
		"1: b 1b\n"
		".endr\n"
#else
		"b reset\n"
		"1: b 1b\n"
		"1: b 1b\n"
		"1: b 1b\n"
		"1: b 1b\n"
		"1: b 1b\n"
		"1: b 1b\n"
		"1: b 1b\n"
#endif
		".word 0x65726162\n"			/* 'bare' */
		".word 0x00786f62\n"			/* 'box' */
		".word _text\n"				/* text base. If copied there,
							 * barebox can skip relocation
							 */
		".word _barebox_image_size\n"		/* image size to copy */
	);
}

#endif /* __ASM_ARM_HEAD_H */
