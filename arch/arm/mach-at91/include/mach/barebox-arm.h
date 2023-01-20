/* SPDX-License-Identifier: GPL-2.0 */
#ifndef AT91_BAREBOX_ARM_H_
#define AT91_BAREBOX_ARM_H_

#include <asm/barebox-arm.h>
#include <asm/common.h>
#include <mach/sama5d3.h>
#include <mach/sama5d4.h>

#define SAMA5_ENTRY_FUNCTION(name, stack_top, r4)				\
	void name (u32 r0, u32 r1, u32 r2, u32 r3);				\
										\
	static void __##name(u32);						\
										\
	void __naked __section(.text_head_entry_##name)	name			\
				(u32 r0, u32 r1, u32 r2, u32 r3)		\
		{								\
			register u32 r4 asm("r4");				\
			__barebox_arm_head();					\
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

#endif
