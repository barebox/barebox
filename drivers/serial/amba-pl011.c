/*
 * (C) Copyright 2000
 * Rob Taylor, Flying Pig Systems. robt@flyingpig.com.
 *
 * (C) Copyright 2004
 * ARM Ltd.
 * Philippe Robin, <philippe.robin@arm.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/* Simple U-Boot driver for the PrimeCell PL010/PL011 UARTs */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <io.h>
#include <regulator.h>
#include <linux/amba/serial.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/amba/bus.h>

/*
 * We wrap our port structure around the generic console_device.
 */
struct amba_uart_port {
	void __iomem		*base;
	struct console_device	uart;		/* uart */
	struct clk		*clk;		/* uart clock */
	u32			uartclk;
	struct vendor_data	*vendor;
};

/* There is by now at least one vendor with differing details, so handle it */
struct vendor_data {
	unsigned int		lcrh_tx;
	unsigned int		lcrh_rx;
};

static struct vendor_data vendor_arm = {
	.lcrh_tx		= UART011_LCRH,
	.lcrh_rx		= UART011_LCRH,
};

static struct vendor_data vendor_st = {
	.lcrh_tx		= ST_UART011_LCRH_TX,
	.lcrh_rx		= ST_UART011_LCRH_RX,
};

static inline struct amba_uart_port *
to_amba_uart_port(struct console_device *uart)
{
	return container_of(uart, struct amba_uart_port, uart);
}

static int pl011_setbaudrate(struct console_device *cdev, int baudrate)
{
	struct amba_uart_port *uart = to_amba_uart_port(cdev);
	unsigned int temp;
	unsigned int divider;
	unsigned int remainder;
	unsigned int fraction;

	/*
	 ** Set baud rate
	 **
	 ** IBRD = UART_CLK / (16 * BAUD_RATE)
	 ** FBRD = ROUND((64 * MOD(UART_CLK,(16 * BAUD_RATE))) / (16 * BAUD_RATE))
	 */
	temp = 16 * baudrate;
	divider = uart->uartclk / temp;
	remainder = uart->uartclk % temp;
	temp = (8 * remainder) / baudrate;
	fraction = (temp >> 1) + (temp & 1);

	writel(divider, uart->base + UART011_IBRD);
	writel(fraction, uart->base + UART011_FBRD);

	return 0;
}

static void pl011_putc(struct console_device *cdev, char c)
{
	struct amba_uart_port *uart = to_amba_uart_port(cdev);

	/* Wait until there is space in the FIFO */
	while (readl(uart->base + UART01x_FR) & UART01x_FR_TXFF);

	/* Send the character */
	writel(c, uart->base + UART01x_DR);
}

static int pl011_getc(struct console_device *cdev)
{
	struct amba_uart_port *uart = to_amba_uart_port(cdev);
	unsigned int data;

	/* Wait until there is data in the FIFO */
	while (readl(uart->base + UART01x_FR) & UART01x_FR_RXFE);

	data = readl(uart->base + UART01x_DR);

	/* Check for an error flag */
	if (data & 0xffffff00) {
		/* Clear the error */
		writel(0xffffffff, uart->base + UART01x_ECR);
		return -1;
	}

	return (int)data;
}

static int pl011_tstc(struct console_device *cdev)
{
	struct amba_uart_port *uart = to_amba_uart_port(cdev);

	return !(readl(uart->base + UART01x_FR) & UART01x_FR_RXFE);
}

static void pl011_rlcr(struct amba_uart_port *uart, u32 lcr)
{
	struct vendor_data	*vendor = uart->vendor;

	writew(lcr, uart->base + vendor->lcrh_rx);
	if (vendor->lcrh_tx != vendor->lcrh_rx) {
		int i;
		/*
		 * Wait 10 PCLKs before writing LCRH_TX register,
		 * to get this delay write read only register 10 times
		 */
		for (i = 0; i < 10; ++i)
			writew(0xff, uart->base + UART011_MIS);
		writew(lcr, uart->base +  vendor->lcrh_tx);
	}
}

static int pl011_init_port(struct console_device *cdev)
{
	struct amba_uart_port *uart = to_amba_uart_port(cdev);

	/*
	 ** First, disable everything.
	 */
	writel(0x0, uart->base + UART011_CR);

	/*
	 * Try to enable the clock producer.
	 */
	clk_enable(uart->clk);

	uart->uartclk = clk_get_rate(uart->clk);

	/*
	 ** Set the UART to be 8 bits, 1 stop bit, no parity, fifo enabled.
	 */
	pl011_rlcr(uart, UART01x_LCRH_WLEN_8 | UART01x_LCRH_FEN);

	/*
	 ** Finally, enable the UART
	 */
	writel((UART01x_CR_UARTEN | UART011_CR_TXE | UART011_CR_RXE | UART011_CR_RTS),
	       uart->base + UART011_CR);

	return 0;
}

static int pl011_probe(struct amba_device *dev, const struct amba_id *id)
{
	struct amba_uart_port *uart;
	struct console_device *cdev;
	struct regulator *r;

	r = regulator_get(&dev->dev, NULL);
	if (!IS_ERR(r)) {
		int ret;

		ret = regulator_enable(r);
		if (ret)
			return ret;
	}

	uart = xzalloc(sizeof(struct amba_uart_port));
	uart->clk = clk_get(&dev->dev, NULL);
	uart->base = amba_get_mem_region(dev);
	uart->vendor = (void*)id->data;

	if (IS_ERR(uart->clk))
		return PTR_ERR(uart->clk);

	cdev = &uart->uart;
	cdev->dev = &dev->dev;
	cdev->tstc = pl011_tstc;
	cdev->putc = pl011_putc;
	cdev->getc = pl011_getc;
	cdev->setbrg = pl011_setbaudrate;

	pl011_init_port(cdev);

	/* Enable UART */

	console_register(cdev);

	return 0;
}

static struct amba_id pl011_ids[] = {
	{
		.id	= 0x00041011,
		.mask	= 0x000fffff,
		.data	= &vendor_arm,
	},
	{
		.id	= 0x00380802,
		.mask	= 0x00ffffff,
		.data	= &vendor_st,
	},
	{ 0, 0 },
};

struct amba_driver pl011_driver = {
	.drv = {
		.name = "uart-pl011",
	},
	.probe		= pl011_probe,
	.id_table	= pl011_ids,
};

static int pl011_init(void)
{
	amba_driver_register(&pl011_driver);
	return 0;
}

console_initcall(pl011_init);
