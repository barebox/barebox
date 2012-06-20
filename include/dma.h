/*
 * Copyright (C) 2012 by Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * This file is released under the GPLv2
 *
 */

#ifndef __DMA_H
#define __DMA_H

#include <malloc.h>
#include <xfuncs.h>

#include <asm/dma.h>

#ifndef dma_alloc
static inline void *dma_alloc(size_t size)
{
	return xmalloc(size);
}
#endif

#ifndef dma_free
static inline void dma_free(void *mem)
{
	free(mem);
}
#endif

#endif /* __DMA_H */
