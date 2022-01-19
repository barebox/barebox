/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _PPC_TYPES_H
#define _PPC_TYPES_H

#include <asm-generic/int-ll64.h>

#ifndef __ASSEMBLY__

typedef struct {
	__u32 u[4];
} __attribute((aligned(16))) vector128;

#endif /* __ASSEMBLY__ */

#endif
