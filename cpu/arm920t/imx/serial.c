/*
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
#include <asm/arch/imx-regs.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>

#define URXD0(base) __REG( 0x0 +(base))  /* Receiver Register */
#define URTX0(base) __REG( 0x40 +(base)) /* Transmitter Register */
#define UCR1(base)  __REG( 0x80 +(base)) /* Control Register 1 */
#define UCR2(base)  __REG( 0x84 +(base)) /* Control Register 2 */
#define UCR3(base)  __REG( 0x88 +(base)) /* Control Register 3 */
#define UCR4(base)  __REG( 0x8c +(base)) /* Control Register 4 */
#define UFCR(base)  __REG( 0x90 +(base)) /* FIFO Control Register */
#define USR1(base)  __REG( 0x94 +(base)) /* Status Register 1 */
#define USR2(base)  __REG( 0x98 +(base)) /* Status Register 2 */
#define UESC(base)  __REG( 0x9c +(base)) /* Escape Character Register */
#define UTIM(base)  __REG( 0xa0 +(base)) /* Escape Timer Register */
#define UBIR(base)  __REG( 0xa4 +(base)) /* BRM Incremental Register */
#define UBMR(base)  __REG( 0xa8 +(base)) /* BRM Modulator Register */
#define UBRC(base)  __REG( 0xac +(base)) /* Baud Rate Count Register */
#define BIPR1(base) __REG( 0xb0 +(base)) /* Incremental Preset Register 1 */
#define BIPR2(base) __REG( 0xb4 +(base)) /* Incremental Preset Register 2 */
#define BIPR3(base) __REG( 0xb8 +(base)) /* Incremental Preset Register 3 */
#define BIPR4(base) __REG( 0xbc +(base)) /* Incremental Preset Register 4 */
#define BMPR1(base) __REG( 0xc0 +(base)) /* BRM Modulator Register 1 */
#define BMPR2(base) __REG( 0xc4 +(base)) /* BRM Modulator Register 2 */
#define BMPR3(base) __REG( 0xc8 +(base)) /* BRM Modulator Register 3 */
#define BMPR4(base) __REG( 0xcc +(base)) /* BRM Modulator Register 4 */
#define UTS(base)   __REG( 0xd0 +(base)) /* UART Test Register */

#define  UTS_RXEMPTY	 (1<<5)	/* RxFIFO empty */
#define  UTS_TXFULL 	 (1<<4)	/* TxFIFO full */
#define  UTS_TXEMPTY	 (1<<6)	/* TxFIFO empty */

extern void imx_gpio_mode(int gpio_mode);

/*
 * Initialise the serial port with the given baudrate. The settings
 * are always 8 data bits, no parity, 1 stop bit, no start bits.
 *
 */
static int imx_serial_init_port(struct console_device *cdev)
{
#if 0
	volatile struct imx_serial* base = (struct imx_serial *)UART_BASE;
#ifdef CONFIG_IMX_SERIAL1
	imx_gpio_mode(PC11_PF_UART1_TXD);
	imx_gpio_mode(PC12_PF_UART1_RXD);
#else
	imx_gpio_mode(PB30_PF_UART2_TXD);
	imx_gpio_mode(PB31_PF_UART2_RXD);
#endif

	/* Disable UART */
	base->ucr1 &= ~UCR1_UARTEN;

	/* Set to default POR state */

	base->ucr1 = 0x00000004;
	base->ucr2 = 0x00000000;
	base->ucr3 = 0x00000000;
	base->ucr4 = 0x00008040;
	base->uesc = 0x0000002B;
	base->utim = 0x00000000;
	base->ubir = 0x00000000;
	base->ubmr = 0x00000000;
	base->uts  = 0x00000000;
	/* Set clocks */
	base->ucr4 |= UCR4_REF16;

	/* Configure FIFOs */
	base->ufcr = 0xa81;

	/* Set the numerator value minus one of the BRM ratio */
	base->ubir = (CONFIG_BAUDRATE / 100) - 1;

	/* Set the denominator value minus one of the BRM ratio	*/
	base->ubmr = 10000 - 1;

	/* Set to 8N1 */
	base->ucr2 &= ~UCR2_PREN;
	base->ucr2 |= UCR2_WS;
	base->ucr2 &= ~UCR2_STPB;

	/* Ignore RTS */
	base->ucr2 |= UCR2_IRTS;

	/* Enable UART */
	base->ucr1 |= UCR1_UARTEN | UCR1_UARTCLKEN;

	/* Enable FIFOs */
	base->ucr2 |= UCR2_SRST | UCR2_RXEN | UCR2_TXEN;

  	/* Clear status flags */
	base->usr2 |= USR2_ADET  |
	          USR2_DTRF  |
	          USR2_IDLE  |
	          USR2_IRINT |
	          USR2_WAKE  |
	          USR2_RTSF  |
	          USR2_BRCD  |
	          USR2_ORE   |
	          USR2_RDR;

  	/* Clear status flags */
	base->usr1 |= USR1_PARITYERR |
	          USR1_RTSD      |
	          USR1_ESCF      |
	          USR1_FRAMERR   |
	          USR1_AIRINT    |
	          USR1_AWAKE;
#endif
	return 0;
}

static void imx_serial_putc(struct console_device *cdev, char c)
{
	struct device_d *dev = cdev->dev;

	/* Wait for Tx FIFO not full */
	while (UTS(dev->map_base) & UTS_TXFULL);

        URTX0(dev->map_base) = c;
}

static int imx_serial_tstc(struct console_device *cdev)
{
	struct device_d *dev = cdev->dev;

	/* If receive fifo is empty, return false */
	if (UTS(dev->map_base) & UTS_RXEMPTY)
		return 0;
	return 1;
}

static int imx_serial_getc(struct console_device *cdev)
{
	struct device_d *dev = cdev->dev;

	unsigned char ch;

	while(UTS(dev->map_base) & UTS_RXEMPTY);

	ch = URXD0(dev->map_base);

	return ch;
}

static int imx_serial_probe(struct device_d *dev)
{
	struct console_device *cdev;

	cdev = malloc(sizeof(struct console_device));
	dev->type_data = cdev;
	cdev->dev = dev;
	cdev->flags = CONSOLE_STDIN | CONSOLE_STDOUT | CONSOLE_STDERR;
	cdev->tstc = imx_serial_tstc;
	cdev->putc = imx_serial_putc;
	cdev->getc = imx_serial_getc;

	imx_serial_init_port(cdev);

	console_register(cdev);

	return 0;
}

static struct driver_d imx_serial_driver = {
        .name  = "imx_serial",
        .probe = imx_serial_probe,
        .type  = DEVICE_TYPE_CONSOLE,
};

static int imx_serial_init(void)
{
	register_driver(&imx_serial_driver);
	return 0;
}

console_initcall(imx_serial_init);
