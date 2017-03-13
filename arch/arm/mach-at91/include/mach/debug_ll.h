/*
 * Copyright (C) 2012
 * Jean-Christophe PLAGNIOL-VILLARD <planioj@jcrosoft.com>
 *
 * Under GPLv2
 */

#ifndef __MACH_DEBUG_LL_H__
#define __MACH_DEBUG_LL_H__

#include <asm/io.h>
#include <mach/hardware.h>

#ifdef CONFIG_HAVE_AT91_DBGU0
#define UART_BASE	AT91_BASE_DBGU0
#else
#define UART_BASE	AT91_BASE_DBGU1
#endif

#define ATMEL_US_CSR		0x0014
#define ATMEL_US_THR		0x001c
#define ATMEL_US_TXRDY		(1 << 1)
#define ATMEL_US_TXEMPTY	(1 << 9)

/*
 * The following code assumes the serial port has already been
 * initialized by the bootloader.  If you didn't setup a port in
 * your bootloader then nothing will appear (which might be desired).
 *
 * This does not append a newline
 */
static inline void PUTC_LL(char c)
{
	while (!(__raw_readl(UART_BASE + ATMEL_US_CSR) & ATMEL_US_TXRDY))
		barrier();
	__raw_writel(c, UART_BASE + ATMEL_US_THR);

	while (!(__raw_readl(UART_BASE + ATMEL_US_CSR) & ATMEL_US_TXEMPTY))
		barrier();
}
#endif
