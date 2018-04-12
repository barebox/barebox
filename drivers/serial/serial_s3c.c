/*
 * (c) 2009...2011 Juergen Beisert <j.beisert@pengutronix.de>
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
 *
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <io.h>
#include <mach/s3c-generic.h>
#include <mach/s3c-iomap.h>

/* Note: Offsets are for little endian access */
#define ULCON 0x00		/* line control */
#define UCON 0x04		/* UART control */
# define UCON_SET_CLK_SRC(x) (((x) & 0x03) << 10)
# define UCON_GET_CLK_SRC(x) (((x) >> 10) & 0x03)
#define UFCON 0x08		/* FIFO control */
#define UMCON 0x0c		/* modem control */
#define UTRSTAT 0x10		/* Rx/Tx status */
#define UERSTAT 0x14		/* error status */
#define UFSTAT 0x18		/* FIFO status */
#define UMSTAT 0x1c		/* modem status */
#define UTXH 0x20		/* transmitt */
#define URXH 0x24		/* receive */
#define UBRDIV 0x28		/* baudrate generator */
#define UBRDIVSLOT 0x2c		/* baudrate slot generator */
#define UINTM 0x38		/* interrupt mask register */

struct s3c_uart {
	void __iomem *regs;
	struct console_device cdev;
};

#define to_s3c_uart(c)	container_of(c, struct s3c_uart, cdev)

/* each architecture has a preferred reference clock for its UARTs */
static unsigned s3c_select_arch_input_clock(void)
{
	/* S3C24xx: 0=2=PCLK, 1=UEXTCLK, 3=FCLK/n */
	if (IS_ENABLED(CONFIG_ARCH_S3C24xx))
		return 0;	/* use the internal PCLK */
	/* S3C64xx: 0=2=PCLK, 1=UCLK0, 3=UCLK1 */
	if (IS_ENABLED(CONFIG_ARCH_S3C64xx))
		return 3;	/* use the internal UCLK1 */
	/* S5PCxx: 0=PCLK, 1=SCLK_UART */
	if (IS_ENABLED(CONFIG_ARCH_S5PCxx))
		return 0;	/* use the internal PCLK */
}

static unsigned s3c_get_arch_uart_input_clock(void __iomem *base)
{
	unsigned reg = readw(base + UCON);
	return s3c_get_uart_clk(UCON_GET_CLK_SRC(reg));
}

/*
 * This table takes the fractional value of the baud divisor and gives
 * the recommended setting for the UDIVSLOT register. Refer the datasheet
 * for further details
 */
static const uint16_t udivslot_table[] __maybe_unused = {
	0x0000, 0x0080, 0x0808, 0x0888, 0x2222, 0x4924, 0x4A52, 0x54AA,
	0x5555, 0xD555, 0xD5D5, 0xDDD5, 0xDDDD, 0xDFDD, 0xDFDF, 0xFFDF,
};

static int s3c_serial_setbaudrate(struct console_device *cdev, int baudrate)
{
	struct s3c_uart *priv = to_s3c_uart(cdev);
	void __iomem *base = priv->regs;
	unsigned val;

	if (IS_ENABLED(CONFIG_DRIVER_SERIAL_S3C_IMPROVED)) {
		val = s3c_get_arch_uart_input_clock(base) / baudrate;
		writew(udivslot_table[val & 15], base + UBRDIVSLOT);
	}

	val = s3c_get_arch_uart_input_clock(base) / (16 * baudrate) - 1;
	writew(val, base + UBRDIV);

	return 0;
}

static int s3c_serial_init_port(struct console_device *cdev)
{
	struct s3c_uart *priv = to_s3c_uart(cdev);
	void __iomem *base = priv->regs;

	/* FIFO enable, Tx/Rx FIFO clear */
	writeb(0x07, base + UFCON);
	writeb(0x00, base + UMCON);

	/* Normal,No parity,1 stop,8 bit */
	writeb(0x03, base + ULCON);

	/*
	 * S3C2440 SoC:
	 *  - no clock divider
	 * all SoCs:
	 *  - enable receive and transmit mode
	 */
	writew(0x0005 | UCON_SET_CLK_SRC(s3c_select_arch_input_clock()),
						base + UCON);

	if (IS_ENABLED(CONFIG_DRIVER_SERIAL_S3C_IMPROVED))
		/* 'interrupt or polling mode' for both directions */
		writeb(0xf, base + UINTM);

	if (IS_ENABLED(CONFIG_DRIVER_SERIAL_S3C_AUTOSYNC))
		writeb(0x10, base + UMCON); /* enable auto flow control */
	else
		writeb(0x01, base + UMCON); /* RTS up */

	return 0;
}

static void s3c_serial_putc(struct console_device *cdev, char c)
{
	struct s3c_uart *priv = to_s3c_uart(cdev);
	void __iomem *base = priv->regs;

	/* Wait for Tx FIFO not full */
	while (!(readb(base + UTRSTAT) & 0x2))
		;

	writeb(c, base + UTXH);
}

static int s3c_serial_tstc(struct console_device *cdev)
{
	struct s3c_uart *priv = to_s3c_uart(cdev);
	void __iomem *base = priv->regs;

	/* If receive fifo is empty, return false */
	if (readb(base + UTRSTAT) & 0x1)
		return 1;

	return 0;
}

static int s3c_serial_getc(struct console_device *cdev)
{
	struct s3c_uart *priv = to_s3c_uart(cdev);
	void __iomem *base = priv->regs;

	/* wait for a character */
	while (!(readb(base + UTRSTAT) & 0x1))
		;

	return readb(base + URXH);
}

static void s3c_serial_flush(struct console_device *cdev)
{
	struct s3c_uart *priv = to_s3c_uart(cdev);
	void __iomem *base = priv->regs;

	while (!(readb(base + UTRSTAT) & 0x4))
		;
}

static int s3c_serial_probe(struct device_d *dev)
{
	struct resource *iores;
	struct s3c_uart *priv;
	struct console_device *cdev;

	priv = xzalloc(sizeof(struct s3c_uart));
	cdev = &priv->cdev;
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	priv->regs = IOMEM(iores->start);
	dev->priv = priv;
	cdev->dev = dev;
	cdev->tstc = s3c_serial_tstc;
	cdev->putc = s3c_serial_putc;
	cdev->getc = s3c_serial_getc;
	cdev->flush = s3c_serial_flush;
	cdev->setbrg = s3c_serial_setbaudrate;

	s3c_serial_init_port(cdev);

	/* Enable UART */
	console_register(cdev);

	return 0;
}

static struct driver_d s3c_serial_driver = {
	.name   = "s3c_serial",
	.probe  = s3c_serial_probe,
};
console_platform_driver(s3c_serial_driver);
