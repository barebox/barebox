/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 by Marc Kleine-Budde <mkl@pengutronix.de>
 */

#ifndef __ASM_DMA_H
#define __ASM_DMA_H

#include <common.h>
#include <xfuncs.h>
#include <asm/cpu-info.h>

#define dma_alloc dma_alloc
static inline void *dma_alloc(size_t size)
{
	unsigned long max_linesz = max(current_cpu_data.dcache.linesz,
				       current_cpu_data.scache.linesz);
	return xmemalign(max_linesz, ALIGN(size, max_linesz));
}

#include "asm/dma-mapping.h"

#endif /* __ASM_DMA_H */
