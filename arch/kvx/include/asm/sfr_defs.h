/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2024 Kalray Inc.
 */

#ifndef _ASM_KVX_SFR_DEFS_H_
#define _ASM_KVX_SFR_DEFS_H_

#if defined(CONFIG_ARCH_COOLIDGE_V1)
#include "asm/v1/sfr_defs.h"
#elif defined(CONFIG_ARCH_COOLIDGE_V2)
#include "asm/v2/sfr_defs.h"
#else
#error "Unsupported arch"
#endif

#endif /* _ASM_SFR_DEFS_H_ */
