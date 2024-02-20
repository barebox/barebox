/* SPDX-License-Identifier: GPL-2.0 */
#ifndef AT91_BAREBOX_ARM_H_
#define AT91_BAREBOX_ARM_H_

#include <asm/barebox-arm.h>
#include <asm/common.h>
#include <mach/at91/sama5d3.h>
#include <mach/at91/sama5d4.h>
#include <mach/at91/at91sam9261.h>

#ifdef CONFIG_AT91_LOAD_BAREBOX_SRAM
#define AT91_EXV6	".word _barebox_image_size\n"
#else
#define AT91_EXV6	".word _barebox_bare_init_size\n"
#endif

#include <linux/compiler.h>

static __always_inline void __barebox_at91_head(void)
{
	__asm__ __volatile__ (
#ifdef CONFIG_THUMB2_BAREBOX
#error Thumb2 is not supported
#else
		"b 2f\n"
		"1: b 1b\n"
		"1: b 1b\n"
		"1: b 1b\n"
		"1: b 1b\n"
		AT91_EXV6				/* image size to load by the bootrom */
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

#define SAMA5_ENTRY_FUNCTION(name, stack_top, r4)				\
	void name (u32 r0, u32 r1, u32 r2, u32 r3);				\
										\
	static void __##name(u32);						\
										\
	void __naked __section(.text_head_entry_##name)	name			\
				(u32 r0, u32 r1, u32 r2, u32 r3)		\
		{								\
			register u32 r4 asm("r4");				\
			__barebox_at91_head();					\
			if (stack_top)						\
				arm_setup_stack(stack_top);			\
			__##name(r4);						\
		}								\
		static void noinline __##name(u32 r4)

/* BootROM already initialized usable stack top */
#define SAMA5D2_ENTRY_FUNCTION(name, r4)					\
	SAMA5_ENTRY_FUNCTION(name, 0, r4)

#define SAMA5D3_ENTRY_FUNCTION(name, r4)					\
	SAMA5_ENTRY_FUNCTION(name, SAMA5D3_SRAM_BASE + SAMA5D3_SRAM_SIZE, r4)

#define SAMA5D4_ENTRY_FUNCTION(name, r4)					\
	SAMA5_ENTRY_FUNCTION(name, SAMA5D4_SRAM_BASE + SAMA5D4_SRAM_SIZE, r4)

#define SAM9_ENTRY_FUNCTION(name)	\
	ENTRY_FUNCTION_WITHSTACK_HEAD(name, AT91SAM9261_SRAM_BASE + AT91SAM9261_SRAM_SIZE, \
				      __barebox_at91_head, r0, r1, r2)

#define AT91_ENTRY_FUNCTION(fn, r0, r1, r2)					\
	ENTRY_FUNCTION_WITHSTACK_HEAD(fn, 0, __barebox_at91_head, r0, r1, r2)

#endif
