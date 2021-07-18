/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __ASM_SYSTEM_H_
#define __ASM_SYSTEM_H_

#ifndef __ASSEMBLY__

#define RISCV_MODE_MASK 0x3
enum riscv_mode {
    RISCV_U_MODE	= 0,
    RISCV_S_MODE	= 1,
    RISCV_HS_MODE	= 2,
    RISCV_M_MODE	= 3,
};

static inline enum riscv_mode __riscv_mode(u32 flags)
{
	/* allow non-LTO builds to discard code for unused modes */
	if (!IS_ENABLED(CONFIG_RISCV_MULTI_MODE)) {
		if (IS_ENABLED(CONFIG_RISCV_M_MODE))
			return RISCV_M_MODE;
		if (IS_ENABLED(CONFIG_RISCV_S_MODE))
			return RISCV_S_MODE;
	}

	return flags & RISCV_MODE_MASK;
}

static inline long __riscv_hartid(u32 flags)
{
	long hartid = -1;

	switch (__riscv_mode(flags)) {
	case RISCV_S_MODE:
		__asm__ volatile("mv %0, tp\n" : "=r"(hartid) :);
		break;
	default:
		/* unimplemented */
		break;
	}

	return hartid;
}

#ifndef __PBL__
extern unsigned barebox_riscv_pbl_flags;

static inline enum riscv_mode riscv_mode(void)
{
	return __riscv_mode(barebox_riscv_pbl_flags);
}

static inline long riscv_hartid(void)
{
	return __riscv_hartid(barebox_riscv_pbl_flags);
}
#endif

#endif

#endif
