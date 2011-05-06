/*
 * (C) Copyright 2011, Franck JULLIEN, <elec4fun@gmail.com>
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

#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <asm/io.h>
#include <asm/nios2-io.h>

static int altera_serial_setbaudrate(struct console_device *cdev, int baudrate)
{
	struct nios_uart *uart = (struct nios_uart *)cdev->dev->map_base;
	uint16_t div;

	div = (CPU_FREQ / baudrate) - 1;
	writew(div, &uart->divisor);

	return 0;
}

static void altera_serial_putc(struct console_device *cdev, char c)
{
	struct nios_uart *uart = (struct nios_uart *)cdev->dev->map_base;

	while ((readw(&uart->status) & NIOS_UART_TRDY) == 0);

	writew(c, &uart->txdata);
}

static int altera_serial_tstc(struct console_device *cdev)
{
	struct nios_uart *uart = (struct nios_uart *)cdev->dev->map_base;

	return readw(&uart->status) & NIOS_UART_RRDY;
}

static int altera_serial_getc(struct console_device *cdev)
{
	struct nios_uart *uart = (struct nios_uart *)cdev->dev->map_base;

	while (altera_serial_tstc(cdev) == 0);

	return readw(&uart->rxdata) & 0x000000FF;
}

static int altera_serial_probe(struct device_d *dev)
{
	struct console_device *cdev;

	cdev = xmalloc(sizeof(struct console_device));
	dev->type_data = cdev;
	cdev->dev = dev;
	cdev->f_caps = CONSOLE_STDIN | CONSOLE_STDOUT | CONSOLE_STDERR;
	cdev->tstc = altera_serial_tstc;
	cdev->putc = altera_serial_putc;
	cdev->getc = altera_serial_getc;
	cdev->setbrg = altera_serial_setbaudrate;

	console_register(cdev);

	return 0;
}

static struct driver_d altera_serial_driver = {
	.name = "altera_serial",
	.probe = altera_serial_probe,
};

static int altera_serial_init(void)
{
	return register_driver(&altera_serial_driver);
}

console_initcall(altera_serial_init);

