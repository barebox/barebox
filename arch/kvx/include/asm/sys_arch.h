/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2019 Kalray Inc.
 */

#ifndef _ASM_KVX_SYS_ARCH_H
#define _ASM_KVX_SYS_ARCH_H

#include <asm/sfr_defs.h>

#define EXCEPTION_STRIDE	0x40
#define EXCEPTION_ALIGNMENT	0x100

#define kvx_cluster_id() ((int) \
	((kvx_sfr_get(PCR) & KVX_SFR_PCR_CID_MASK) \
					>> KVX_SFR_PCR_CID_SHIFT))
#define KVX_SFR_START(__sfr_reg) \
	(KVX_SFR_## __sfr_reg ## _SHIFT)

#define KVX_SFR_END(__sfr_reg) \
	(KVX_SFR_## __sfr_reg ## _SHIFT + KVX_SFR_## __sfr_reg ## _WIDTH - 1)

/**
 * Get the value to clear a sfr
 */
#define SFR_CLEAR(__sfr, __field, __lm) \
	KVX_SFR_## __sfr ## _ ## __field ## _ ## __lm ## _CLEAR

#define SFR_CLEAR_WFXL(__sfr, __field)  SFR_CLEAR(__sfr, __field, WFXL)
#define SFR_CLEAR_WFXM(__sfr, __field)  SFR_CLEAR(__sfr, __field, WFXM)

/**
 * Get the value to set a sfr.
 */
#define SFR_SET_WFXL(__sfr, __field, __val) \
	(__val << (KVX_SFR_ ## __sfr ## _ ## __field ## _SHIFT + 32))

#define SFR_SET_WFXM(__sfr, __field, __val) \
	(__val << (KVX_SFR_ ## __sfr ## _ ## __field ## _SHIFT))

/**
 * Generate the mask to clear and set a value using wfx{m|l}.
 */
#define SFR_SET_VAL_WFXL(__sfr, __field, __val) \
	(SFR_SET_WFXL(__sfr, __field, __val) | SFR_CLEAR_WFXL(__sfr, __field))
#define SFR_SET_VAL_WFXM(__sfr, __field, __val) \
	(SFR_SET_WFXM(__sfr, __field, __val) | SFR_CLEAR_WFXM(__sfr, __field))

#endif /* _ASM_KVX_SYS_ARCH_H */
