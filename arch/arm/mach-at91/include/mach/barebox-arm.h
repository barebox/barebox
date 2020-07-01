/* SPDX-License-Identifier: GPL-2.0 */
#ifndef AT91_BAREBOX_ARM_H_
#define AT91_BAREBOX_ARM_H_

#include <asm/barebox-arm.h>

#define SAMA5_ENTRY_FUNCTION(name, r4)						\
	void name (u32 r0, u32 r1, u32 r2, u32 r3);				\
										\
	static void __##name(u32);						\
										\
	void NAKED __section(.text_head_entry_##name)	name			\
				(u32 r0, u32 r1, u32 r2, u32 r3)		\
		{								\
			register u32 r4 asm("r4");				\
			__barebox_arm_head();					\
			__##name(r4);						\
		}								\
		static void NAKED noinline __##name				\
			(u32 r4)
#endif
