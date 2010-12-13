/*
 * (C) Copyright 2010 Juergen Beisert - Pengutronix
 *
 * This code was inspired by some patches made for u-boot covered by:
 * (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>
 * (C) Copyright 2009 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * Note: This is the driver for the debug UART. There is
 * only one of these UARTs on the Freescale/SigmaTel parts
 */

#include <common.h>
#include <init.h>
#include <notifier.h>
#include <gpio.h>
#include <asm/io.h>
#include <mach/imx-regs.h>
#include <mach/clock.h>

#define UARTDBGDR 0x00
#define UARTDBGFR 0x18
# define TXFF (1 << 5)
# define RXFE (1 << 4)
#define UARTDBGIBRD 0x24
#define UARTDBGFBRD 0x28
#define UARTDBGLCR_H 0x2c
# define WLEN8 (3 << 5)
# define WLEN7 (2 << 5)
# define WLEN6 (1 << 5)
# define WLEN5 (0 << 5)
# define FEN (1 << 4)
#define UARTDBGCR 0x30
# define UARTEN (1 << 0)
# define TXE (1 << 8)
# define RXE (1 << 9)
#define UARTDBGIMSC 0x38

struct stm_serial_local {
	struct console_device cdev;
	int baudrate;
	struct notifier_block notify;
};

static void stm_serial_putc(struct console_device *cdev, char c)
{
	struct device_d *dev = cdev->dev;

	/* Wait for room in TX FIFO */
	while (readl(dev->map_base + UARTDBGFR) & TXFF)
		;

	writel(c, dev->map_base + UARTDBGDR);
}

static int stm_serial_tstc(struct console_device *cdev)
{
	struct device_d *dev = cdev->dev;

	/* Check if RX FIFO is not empty */
	return !(readl(dev->map_base + UARTDBGFR) & RXFE);
}

static int stm_serial_getc(struct console_device *cdev)
{
	struct device_d *dev = cdev->dev;

	/* Wait while TX FIFO is empty */
	while (readl(dev->map_base + UARTDBGFR) & RXFE)
		;

	return readl(dev->map_base + UARTDBGDR) & 0xff;
}

static void stm_serial_flush(struct console_device *cdev)
{
	struct device_d *dev = cdev->dev;

	/* Wait for TX FIFO empty */
	while (readl(dev->map_base + UARTDBGFR) & TXFF)
		;
}

static int stm_serial_setbaudrate(struct console_device *cdev, int new_baudrate)
{
	struct device_d *dev = cdev->dev;
	struct stm_serial_local *local = container_of(cdev, struct stm_serial_local, cdev);
	uint32_t cr, lcr_h, quot;

	/* Disable everything */
	cr = readl(dev->map_base + UARTDBGCR);
	writel(0, dev->map_base + UARTDBGCR);

	/* Calculate and set baudrate */
	quot = (imx_get_xclk() * 4) / new_baudrate;
	writel(quot & 0x3f, dev->map_base + UARTDBGFBRD);
	writel(quot >> 6, dev->map_base + UARTDBGIBRD);

	/* Set 8n1 mode, enable FIFOs */
	lcr_h = WLEN8 | FEN;
	writel(lcr_h, dev->map_base + UARTDBGLCR_H);

	/* Re-enable debug UART */
	writel(cr, dev->map_base + UARTDBGCR);

	local->baudrate = new_baudrate;

	return 0;
}

static int stm_clocksource_clock_change(struct notifier_block *nb, unsigned long event, void *data)
{
	struct stm_serial_local *local = container_of(nb, struct stm_serial_local, notify);

	return stm_serial_setbaudrate(&local->cdev, local->baudrate);
}

static int stm_serial_init_port(struct console_device *cdev)
{
	struct device_d *dev = cdev->dev;

	/* Disable UART */
	writel(0, dev->map_base + UARTDBGCR);

	/* Mask interrupts */
	writel(0, dev->map_base + UARTDBGIMSC);

	return 0;
}

static struct stm_serial_local stm_device = {
	.cdev = {
		.f_caps = CONSOLE_STDIN | CONSOLE_STDOUT | CONSOLE_STDERR,
		.tstc = stm_serial_tstc,
		.putc = stm_serial_putc,
		.getc = stm_serial_getc,
		.flush = stm_serial_flush,
		.setbrg = stm_serial_setbaudrate,
	},
};

static int stm_serial_probe(struct device_d *dev)
{
	stm_device.cdev.dev = dev;
	dev->type_data = &stm_device.cdev;

	stm_serial_init_port(&stm_device.cdev);
	stm_serial_setbaudrate(&stm_device.cdev, CONFIG_BAUDRATE);

	/* Enable UART */
	writel(TXE | RXE | UARTEN, dev->map_base + UARTDBGCR);

	console_register(&stm_device.cdev);
	stm_device.notify.notifier_call = stm_clocksource_clock_change;
	clock_register_client(&stm_device.notify);

	return 0;
}

static void stm_serial_remove(struct device_d *dev)
{
	struct console_device *cdev = dev->type_data;

	stm_serial_flush(cdev);
}

static struct driver_d stm_serial_driver = {
        .name   = "stm_serial",
        .probe  = stm_serial_probe,
	.remove = stm_serial_remove,
};

static int stm_serial_init(void)
{
	register_driver(&stm_serial_driver);
	return 0;
}

console_initcall(stm_serial_init);
