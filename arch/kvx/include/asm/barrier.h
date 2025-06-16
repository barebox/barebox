/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2019 Kalray Inc.
 */

#ifndef _ASM_KVX_BARRIER_H
#define _ASM_KVX_BARRIER_H

#include <asm-generic/barrier.h>

/* fence is sufficient to guarantee write ordering */
#define wmb()	__builtin_kvx_fence()

/* no L2 coherency, therefore rmb is D$ invalidation */
#define rmb()   __builtin_kvx_dinval()

/* general memory barrier */
#define mb()    do { wmb(); rmb(); } while (0)

#endif /* _ASM_KVX_BARRIER_H */
