#ifndef __MACH_DEBUG_LL_H__
#define   __MACH_DEBUG_LL_H__

#include <io.h>

#define UART_BASE	0xffc02000

#define LSR_THRE	0x20	/* Xmit holding register empty */
#define LSR		(5 << 2)
#define THR		(0 << 2)

#define LCR_BKSE	0x80	/* Bank select enable */
#define LSR		(5 << 2)
#define THR		(0 << 2)
#define DLL		(0 << 2)
#define IER		(1 << 2)
#define DLM		(1 << 2)
#define FCR		(2 << 2)
#define LCR		(3 << 2)
#define MCR		(4 << 2)
#define MDR		(8 << 2)

static inline unsigned int ns16550_calc_divisor(unsigned int clk,
					 unsigned int baudrate)
{
	return (clk / 16 / baudrate);
}

static inline void INIT_LL(void)
{
	unsigned int clk = 100000000;
	unsigned int divisor = clk / 16 / 115200;

	writeb(0x00, UART_BASE + LCR);
	writeb(0x00, UART_BASE + IER);
	writeb(0x07, UART_BASE + MDR);
	writeb(LCR_BKSE, UART_BASE + LCR);
	writeb(divisor & 0xff, UART_BASE + DLL);
	writeb(divisor >> 8, UART_BASE + DLM);
	writeb(0x03, UART_BASE + LCR);
	writeb(0x03, UART_BASE + MCR);
	writeb(0x07, UART_BASE + FCR);
	writeb(0x00, UART_BASE + MDR);
}

static inline void PUTC_LL(char c)
{
	/* Wait until there is space in the FIFO */
	while ((readb(UART_BASE + LSR) & LSR_THRE) == 0);
	/* Send the character */
	writeb(c, UART_BASE + THR);
	/* Wait to make sure it hits the line, in case we die too soon. */
	while ((readb(UART_BASE + LSR) & LSR_THRE) == 0);
}
#endif
