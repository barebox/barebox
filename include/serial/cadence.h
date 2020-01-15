#ifndef __CADENCE_UART_H__
#define __CADENCE_UART_H__

#define CADENCE_UART_CONTROL		0x00
#define CADENCE_UART_MODE		0x04
#define CADENCE_UART_BAUD_GEN		0x18
#define CADENCE_UART_CHANNEL_STS	0x2C
#define CADENCE_UART_RXTXFIFO		0x30
#define CADENCE_UART_BAUD_DIV		0x34

#define CADENCE_CTRL_RXRES		(1 << 0)
#define CADENCE_CTRL_TXRES		(1 << 1)
#define CADENCE_CTRL_RXEN		(1 << 2)
#define CADENCE_CTRL_RXDIS		(1 << 3)
#define CADENCE_CTRL_TXEN		(1 << 4)
#define CADENCE_CTRL_TXDIS		(1 << 5)
#define CADENCE_CTRL_RSTTO		(1 << 6)
#define CADENCE_CTRL_STTBRK		(1 << 7)
#define CADENCE_CTRL_STPBRK		(1 << 8)

#define CADENCE_MODE_CLK_REF		(0 << 0)
#define CADENCE_MODE_CLK_REF_DIV	(1 << 0)
#define CADENCE_MODE_CHRL_6		(3 << 1)
#define CADENCE_MODE_CHRL_7		(2 << 1)
#define CADENCE_MODE_CHRL_8		(0 << 1)
#define CADENCE_MODE_PAR_EVEN		(0 << 3)
#define CADENCE_MODE_PAR_ODD		(1 << 3)
#define CADENCE_MODE_PAR_SPACE		(2 << 3)
#define CADENCE_MODE_PAR_MARK		(3 << 3)
#define CADENCE_MODE_PAR_NONE		(4 << 3)

#define CADENCE_STS_REMPTY		(1 << 1)
#define CADENCE_STS_RFUL		(1 << 2)
#define CADENCE_STS_TEMPTY		(1 << 3)
#define CADENCE_STS_TFUL		(1 << 4)

static inline void cadence_uart_init(void __iomem *uartbase)
{
	int baudrate = CONFIG_BAUDRATE;
	unsigned int clk = 49999995;
	unsigned int gen, div;

	/* disable transmitter and receiver */
	writel(0, uartbase + CADENCE_UART_CONTROL);

	/* calculate and set baud clock generator parameters */
	for (div = 4; div < 256; div++) {
		int calc_rate, error;

		gen = clk / (baudrate * (div + 1));

		if (gen < 1 || gen > 65535)
			continue;

		calc_rate = clk / (gen * (div + 1));
		error = baudrate - calc_rate;
		if (error < 0)
			error *= -1;
		if (((error * 100) / baudrate) < 3)
			break;
	}

	writel(gen, uartbase + CADENCE_UART_BAUD_GEN);
	writel(div, uartbase + CADENCE_UART_BAUD_DIV);

	/* soft-reset tx/rx paths */
	writel(CADENCE_CTRL_RXRES | CADENCE_CTRL_TXRES,
	       uartbase + CADENCE_UART_CONTROL);

	while (readl(uartbase + CADENCE_UART_CONTROL) &
		(CADENCE_CTRL_RXRES | CADENCE_CTRL_TXRES))
		;

	/* enable UART */
	writel(CADENCE_MODE_CLK_REF | CADENCE_MODE_CHRL_8 |
	       CADENCE_MODE_PAR_NONE, uartbase + CADENCE_UART_MODE);
	writel(CADENCE_CTRL_RXEN | CADENCE_CTRL_TXEN,
	       uartbase + CADENCE_UART_CONTROL);
}

static inline void cadence_uart_putc(void *base, int c)
{
	if (!(readl(base + CADENCE_UART_CONTROL) & CADENCE_CTRL_TXEN))
		return;

	while ((readl(base + CADENCE_UART_CHANNEL_STS) & CADENCE_STS_TFUL))
		;

	writel(c, base + CADENCE_UART_RXTXFIFO);
}

#endif /* __CADENCE_UART_H__ */
