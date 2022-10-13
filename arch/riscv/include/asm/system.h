/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __ASM_SYSTEM_H_
#define __ASM_SYSTEM_H_

#ifndef __ASSEMBLY__

#include <asm/sbi.h>

#define RISCV_MODE_MASK 0x3
enum riscv_mode {
    RISCV_U_MODE	= 0,
    RISCV_S_MODE	= 1,
    RISCV_HS_MODE	= 2,
    RISCV_M_MODE	= 3,
};

static inline void riscv_set_flags(unsigned flags)
{
	switch (flags & RISCV_MODE_MASK) {
	case RISCV_S_MODE:
		__asm__ volatile("csrw sscratch, %0" : : "r"(flags));
		break;
	case RISCV_M_MODE:
		__asm__ volatile("csrw mscratch, %0" : : "r"(flags));
		break;
	default:
		/* Other modes are not implemented yet */
		break;
	}
}

static inline u32 riscv_get_flags(void)
{
	u32 flags = 0;

	if (IS_ENABLED(CONFIG_RISCV_S_MODE))
		__asm__ volatile("csrr %0, sscratch" : "=r"(flags));

	/*
	 * Since we always set the scratch register on the very beginning, a
	 * empty flags indicates that we are running in M-mode.
	 */
	if (!flags)
		__asm__ volatile("csrr %0, mscratch" : "=r"(flags));

	return flags;
}

static inline enum riscv_mode riscv_mode(void)
{
	/* allow non-LTO builds to discard code for unused modes */
	if (!IS_ENABLED(CONFIG_RISCV_MULTI_MODE)) {
		if (IS_ENABLED(CONFIG_RISCV_M_MODE))
			return RISCV_M_MODE;
		if (IS_ENABLED(CONFIG_RISCV_S_MODE))
			return RISCV_S_MODE;
	}

	return riscv_get_flags() & RISCV_MODE_MASK;
}

static inline long riscv_hartid(void)
{
	long hartid = -1;

	switch (riscv_mode()) {
	case RISCV_S_MODE:
		__asm__ volatile("mv %0, tp\n" : "=r"(hartid) :);
		break;
	default:
		/* unimplemented */
		break;
	}

	return hartid;
}

static inline long riscv_vendor_id(void)
{
	struct sbiret ret;
	long id;

	switch (riscv_mode()) {
	case RISCV_M_MODE:
		__asm__ volatile("csrr %0, mvendorid\n" : "=r"(id));
		return id;
	case RISCV_S_MODE:
		/*
		 * We need to use the sbi_ecall() since it can be that we got
		 * called without a working stack
		 */
		ret = sbi_ecall(SBI_EXT_BASE, SBI_EXT_BASE_GET_MVENDORID,
				0, 0, 0, 0, 0, 0);
		if (!ret.error)
			return ret.value;
		return -1;
	default:
		return -1;
	}
}

#endif

#endif
