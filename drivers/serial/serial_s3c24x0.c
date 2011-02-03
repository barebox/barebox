/*
 * (c) 2009 Juergen Beisert <j.beisert@saschahauer.de>
 *
 * Based on code from:
 * (c) 2004 Sascha Hauer <sascha@saschahauer.de>
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

#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <asm/io.h>
#include <mach/s3c24xx-generic.h>
#include <mach/s3c24x0-iomap.h>

/* Note: Offsets are for little endian access */
#define ULCON 0x00		/* line control */
#define UCON 0x04		/* UART control */
#define UFCON 0x08		/* FIFO control */
#define UMCON 0x0c		/* modem control */
#define UTRSTAT 0x10		/* Rx/Tx status */
#define UERSTAT 0x14		/* error status */
#define UFSTAT 0x18		/* FIFO status */
#define UMSTAT 0x1c		/* modem status */
#define UTXH 0x20		/* transmitt */
#define URXH 0x24		/* receive */
#define UBRDIV 0x28		/* baudrate generator */

static int s3c24x0_serial_setbaudrate(struct console_device *cdev, int baudrate)
{
	struct device_d *dev = cdev->dev;
	unsigned val;

	/* value is calculated so : PCLK / (16 * baudrate) -1 */
	val = s3c24xx_get_pclk() / (16 * baudrate) - 1;
	writew(val, dev->map_base + UBRDIV);

	return 0;
}

static int s3c24x0_serial_init_port(struct console_device *cdev)
{
	struct device_d *dev = cdev->dev;

	/* FIFO enable, Tx/Rx FIFO clear */
	writeb(0x07, dev->map_base + UFCON);
	writeb(0x00, dev->map_base + UMCON);

	/* Normal,No parity,1 stop,8 bit */
	writeb(0x03, dev->map_base + ULCON);
	/*
	 * tx=level,rx=edge,disable timeout int.,enable rx error int.,
	 * normal,interrupt or polling
	 */
	writew(0x0245, dev->map_base + UCON);

#ifdef CONFIG_DRIVER_SERIAL_S3C24X0_AUTOSYNC
	writeb(0x01, dev->map_base + UMCON); /* RTS up */
#endif

	return 0;
}

static void s3c24x0_serial_putc(struct console_device *cdev, char c)
{
	struct device_d *dev = cdev->dev;

	/* Wait for Tx FIFO not full */
	while (!(readb(dev->map_base + UTRSTAT) & 0x2))
		;

	writeb(c, dev->map_base + UTXH);
}

static int s3c24x0_serial_tstc(struct console_device *cdev)
{
	struct device_d *dev = cdev->dev;

	/* If receive fifo is empty, return false */
	if (readb(dev->map_base + UTRSTAT) & 0x1)
		return 1;

	return 0;
}

static int s3c24x0_serial_getc(struct console_device *cdev)
{
	struct device_d *dev = cdev->dev;

	/* wait for a character */
	while (!(readb(dev->map_base + UTRSTAT) & 0x1))
		;

	return readb(dev->map_base + URXH);
}

static void s3c24x0_serial_flush(struct console_device *cdev)
{
	struct device_d *dev = cdev->dev;

	while (!(readb(dev->map_base + UTRSTAT) & 0x4))
		;
}

static int s3c24x0_serial_probe(struct device_d *dev)
{
	struct console_device *cdev;

	cdev = xmalloc(sizeof(struct console_device));
	dev->type_data = cdev;
	cdev->dev = dev;
	cdev->f_caps = CONSOLE_STDIN | CONSOLE_STDOUT | CONSOLE_STDERR;
	cdev->tstc = s3c24x0_serial_tstc;
	cdev->putc = s3c24x0_serial_putc;
	cdev->getc = s3c24x0_serial_getc;
	cdev->flush = s3c24x0_serial_flush;
	cdev->setbrg = s3c24x0_serial_setbaudrate;

	s3c24x0_serial_init_port(cdev);

	/* Enable UART */
	console_register(cdev);

	return 0;
}

static void s3c24x0_serial_remove(struct device_d *dev)
{
	struct console_device *cdev = dev->type_data;

	s3c24x0_serial_flush(cdev);
	free(cdev);
	dev->type_data = NULL;
}

static struct driver_d s3c24x0_serial_driver = {
	.name   = "s3c24x0_serial",
	.probe  = s3c24x0_serial_probe,
	.remove = s3c24x0_serial_remove,
};

static int s3c24x0_serial_init(void)
{
	register_driver(&s3c24x0_serial_driver);
	return 0;
}

console_initcall(s3c24x0_serial_init);
