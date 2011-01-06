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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/* Simple U-Boot driver for the PrimeCell PL010/PL011 UARTs */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <asm/io.h>
#include <linux/amba/serial.h>
#include <linux/clk.h>
#include <linux/err.h>

/*
 * We wrap our port structure around the generic console_device.
 */
struct amba_uart_port {
	struct console_device	uart;		/* uart */
	struct clk		*clk;		/* uart clock */
	u32			uartclk;
};

static inline struct amba_uart_port *
to_amba_uart_port(struct console_device *uart)
{
	return container_of(uart, struct amba_uart_port, uart);
}

static int pl011_setbaudrate(struct console_device *cdev, int baudrate)
{
	struct device_d *dev = cdev->dev;
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

	writel(divider, dev->map_base + UART011_IBRD);
	writel(fraction, dev->map_base + UART011_FBRD);

	return 0;
}

static void pl011_putc(struct console_device *cdev, char c)
{
	struct device_d *dev = cdev->dev;

	/* Wait until there is space in the FIFO */
	while (readl(dev->map_base + UART01x_FR) & UART01x_FR_TXFF);

	/* Send the character */
	writel(c, dev->map_base + UART01x_DR);
}

static int pl011_getc(struct console_device *cdev)
{
	struct device_d *dev = cdev->dev;
	unsigned int data;

	/* Wait until there is data in the FIFO */
	while (readl(dev->map_base + UART01x_FR) & UART01x_FR_RXFE);

	data = readl(dev->map_base + UART01x_DR);

	/* Check for an error flag */
	if (data & 0xffffff00) {
		/* Clear the error */
		writel(0xffffffff, dev->map_base + UART01x_ECR);
		return -1;
	}

	return (int)data;
}

static int pl011_tstc(struct console_device *cdev)
{
	struct device_d *dev = cdev->dev;

	return !(readl(dev->map_base + UART01x_FR) & UART01x_FR_RXFE);
}

int pl011_init_port (struct console_device *cdev)
{
	struct device_d *dev = cdev->dev;
	struct amba_uart_port *uart = to_amba_uart_port(cdev);

	/*
	 ** First, disable everything.
	 */
	writel(0x0, dev->map_base + UART011_CR);

	/*
	 * Try to enable the clock producer.
	 */
	clk_enable(uart->clk);

	uart->uartclk = clk_get_rate(uart->clk);

	/*
	 * set baud rate
	 */
	pl011_setbaudrate(cdev, 115200);
	/*
	 ** Set the UART to be 8 bits, 1 stop bit, no parity, fifo enabled.
	 */
	writel((UART01x_LCRH_WLEN_8 | UART01x_LCRH_FEN),
	       dev->map_base + UART011_LCRH);

	/*
	 ** Finally, enable the UART
	 */
	writel((UART01x_CR_UARTEN | UART011_CR_TXE | UART011_CR_RXE),
	       dev->map_base + UART011_CR);

	return 0;
}

static int pl011_probe(struct device_d *dev)
{
	struct amba_uart_port *uart;
	struct console_device *cdev;

	uart = xmalloc(sizeof(struct amba_uart_port));
	uart->clk = clk_get(dev, NULL);

	if (IS_ERR(uart->clk))
		return PTR_ERR(uart->clk);

	cdev = &uart->uart;
	dev->type_data = cdev;
	cdev->dev = dev;
	cdev->f_caps = CONSOLE_STDIN | CONSOLE_STDOUT | CONSOLE_STDERR;
	cdev->tstc = pl011_tstc;
	cdev->putc = pl011_putc;
	cdev->getc = pl011_getc;
	cdev->setbrg = pl011_setbaudrate;

	pl011_init_port(cdev);

	/* Enable UART */

	console_register(cdev);

	return 0;
}

static struct driver_d pl011_driver = {
	.name = "uart-pl011",
	.probe = pl011_probe,
};

static int pl011_init(void)
{
	register_driver(&pl011_driver);
	return 0;
}

console_initcall(pl011_init);
