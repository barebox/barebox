#ifndef __ASM_ARM_HEAD_H
#define __ASM_ARM_HEAD_H

#ifdef CONFIG_HAVE_MACH_ARM_HEAD
#include <mach/barebox-arm-head.h>
#else
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
		".asciz \"barebox\"\n"
		".word _text\n"				/* text base. If copied there,
							 * barebox can skip relocation
							 */
		".word _barebox_image_size\n"		/* image size to copy */
	);
}
#endif

#endif /* __ASM_ARM_HEAD_H */
