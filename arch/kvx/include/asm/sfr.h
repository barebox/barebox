/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2019 Kalray Inc.
 */

#ifndef _ASM_KVX_SFR_H
#define _ASM_KVX_SFR_H

#include <asm/sfr_defs.h>

#define wfxl(_sfr, _val)	__builtin_kvx_wfxl(_sfr, _val)

#define wfxm(_sfr, _val)	__builtin_kvx_wfxm(_sfr, _val)

static inline uint64_t make_sfr_val(uint64_t mask, uint64_t value)
{
	return ((value & 0xFFFFFFFF) << 32) | (mask & 0xFFFFFFFF);
}

static inline void
kvx_sfr_set_mask(unsigned char sfr, uint64_t mask, uint64_t value)
{
	uint64_t wf_val;
	/* Least significant bits */
	if (mask & 0xFFFFFFFF) {
		wf_val = make_sfr_val(mask, value);
		wfxl(sfr, wf_val);
	}

	/* Most significant bits */
	if (mask & (0xFFFFFFFFULL << 32)) {
		value >>= 32;
		mask >>= 32;
		wf_val = make_sfr_val(mask, value);
		wfxm(sfr, wf_val);
	}
}

#define kvx_sfr_set_field(sfr, field, value) \
	kvx_sfr_set_mask(KVX_SFR_ ## sfr, \
		KVX_SFR_ ## sfr ## _ ## field ## _MASK, \
		((uint64_t) (value) << KVX_SFR_ ## sfr ## _ ## field ## _SHIFT))

#define kvx_sfr_set(_sfr, _val)	__builtin_kvx_set(KVX_SFR_ ## _sfr, _val)
#define kvx_sfr_get(_sfr)	__builtin_kvx_get(KVX_SFR_ ## _sfr)

#endif	/* _ASM_KVX_SFR_DEFS_H */
