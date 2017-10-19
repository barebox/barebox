#ifndef __ASM_ARM_HEAD_H
#define __ASM_ARM_HEAD_H

#include <asm/system.h>

#ifndef __ASSEMBLY__

void arm_cpu_lowlevel_init(void);
void cortex_a7_lowlevel_init(void);

/*
 * 32 bytes at this offset is reserved in the barebox head for board/SoC
 * usage
 */
#define ARM_HEAD_SPARE_OFS	0x30
#define ARM_HEAD_SPARE_MARKER	0x55555555

#ifdef CONFIG_HAVE_MACH_ARM_HEAD
#include <mach/barebox-arm-head.h>
#else
static inline void __barebox_arm_head(void)
{
	__asm__ __volatile__ (
#ifdef CONFIG_THUMB2_BAREBOX
		".arm\n"
		"adr r9, 1f + 1\n"
		"bx r9\n"
		".thumb\n"
		"1:\n"
		"bl 2f\n"
		".rept 10\n"
		"1: b 1b\n"
		".endr\n"
#else
		"b 2f\n"
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
		".rept 8\n"
		".word 0x55555555\n"
		".endr\n"
		"2:\n"
	);
}
static inline void barebox_arm_head(void)
{
	__barebox_arm_head();
	__asm__ __volatile__ (
		"b barebox_arm_reset_vector\n"
	);
}
#endif

#endif /* __ASSEMBLY__ */

#endif /* __ASM_ARM_HEAD_H */
