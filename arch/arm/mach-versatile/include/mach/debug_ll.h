/*
 * Copyright (C) 2010 B Labs Ltd
 * Author: Alexey Zaytsev <alexey.zaytsev@gmail.com>
 *
 * barebox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * barebox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __MACH_DEBUG_LL_H__
#define   __MACH_DEBUG_LL_H__

#include <linux/amba/serial.h>
#include <io.h>

static inline void PUTC_LL(char c)
{
	/* Wait until there is space in the FIFO */
	while (readl(0x101F1000 + UART01x_FR) & UART01x_FR_TXFF);

	/* Send the character */
	writel(c, 0x101F1000 + UART01x_DR);

	/* Wait to make sure it hits the line, in case we die too soon. */
	while (readl(0x101F1000 + UART01x_FR) & UART01x_FR_TXFF);
}

#endif
