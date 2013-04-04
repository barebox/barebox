#ifndef __ASM_ARM_HEAD_H
#define __ASM_ARM_HEAD_H

#include <asm/system.h>

#ifndef __ASSEMBLY__

static inline void arm_cpu_lowlevel_init(void)
{
	uint32_t r;

	/* set the cpu to SVC32 mode */
	__asm__ __volatile__("mrs %0, cpsr":"=r"(r));
	r &= ~0x1f;
	r |= 0xd3;
	__asm__ __volatile__("msr cpsr, %0" : : "r"(r));

	/* disable MMU stuff and caches */
	r = get_cr();
	r &= ~(CR_M | CR_C | CR_B | CR_S | CR_R | CR_V);
	r |= CR_I;

#if __LINUX_ARM_ARCH__ >= 6
	r |= CR_U;
	r &= CR_A;
#else
	r |= CR_A;
#endif

#ifdef __ARMEB__
	r |= CR_B;
#endif
	set_cr(r);
}

/*
 * 32 bytes at this offset is reserved in the barebox head for board/SoC
 * usage
 */
#define ARM_HEAD_SPARE_OFS	0x30
#define ARM_HEAD_SPARE_MARKER	0x55555555

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
		"bl barebox_arm_reset_vector\n"
		".rept 10\n"
		"1: b 1b\n"
		".endr\n"
#else
		"b barebox_arm_reset_vector\n"
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
	);
}
#endif

#else

.macro  arm_cpu_lowlevel_init, scratch

	/* set the cpu to SVC32 mode */
	mrs	\scratch, cpsr
	bic	\scratch, \scratch, #0x1f
	orr	\scratch, \scratch, #0xd3
	msr	cpsr, \scratch

#if __LINUX_ARM_ARCH__ >= 7
	isb
#elif __LINUX_ARM_ARCH__ == 6
	mcr	p15, 0, \scratch, c7, c5, 4
#endif

	/* disable MMU stuff and caches */
	mrc p15, 0, \scratch, c1, c0, 0
	bic	\scratch, \scratch , #(CR_M | CR_C | CR_B)
	bic	\scratch, \scratch,	#(CR_S | CR_R | CR_V)
	orr	\scratch, \scratch, #CR_I

#if __LINUX_ARM_ARCH__ >= 6
	orr	\scratch, \scratch, #CR_U
	bic	\scratch, \scratch, #CR_A
#else
	orr	\scratch, \scratch, #CR_A
#endif

#ifdef __ARMEB__
	orr	\scratch, \scratch, #CR_B
#endif

	mcr	p15, 0, \scratch, c1, c0, 0
.endm

#endif /* __ASSEMBLY__ */

#endif /* __ASM_ARM_HEAD_H */
