/**
 * @file
 * @brief NS16550 Driver implementation
 *
 * FileName: drivers/serial/serial_ns16550.c
 *
 * NS16550 support
 * Modified from u-boot drivers/serial.c and drivers/ns16550.c
 * originally from linux source (arch/ppc/boot/ns16550.c)
 * modified to use CFG_ISA_MEM and new defines
 */
/*
 * (C) Copyright 2008
 * Texas Instruments, <www.ti.com>
 * Nishanth Menon <x0nishan@ti.com>
 *
 * (C) Copyright 2000
 * Rob Taylor, Flying Pig Systems. robt@flyingpig.com.
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

#include <common.h>
#include <driver.h>
#include <init.h>
#include <errno.h>
#include <malloc.h>
#include <io.h>

#include "serial_ns16550.h"
#include <ns16550.h>

/*********** Private Functions **********************************/
static int ns16550_setbaudrate(struct console_device *cdev, int baud_rate);

/**
 * @brief read register
 *
 * @param[in] cdev pointer to console device
 * @param[in] offset
 *
 * @return value
 */
static uint32_t ns16550_read(struct console_device *cdev, uint32_t off)
{
	struct device_d *dev = cdev->dev;
	struct NS16550_plat *plat = (struct NS16550_plat *)dev->platform_data;
	int width = dev->resource[0].flags & IORESOURCE_MEM_TYPE_MASK;

	off <<= plat->shift;

	if (plat->reg_read)
		return plat->reg_read((unsigned long)dev->priv, off);

	switch (width) {
	case IORESOURCE_MEM_8BIT:
		return readb(dev->priv + off);
	case IORESOURCE_MEM_16BIT:
		return readw(dev->priv + off);
	case IORESOURCE_MEM_32BIT:
		return readl(dev->priv + off);
	}
	return -1;
}

/**
 * @brief write register
 *
 * @param[in] cdev pointer to console device
 * @param[in] offset
 * @param[in] val
 */
static void ns16550_write(struct console_device *cdev, uint32_t val,
			  uint32_t off)
{
	struct device_d *dev = cdev->dev;
	struct NS16550_plat *plat = (struct NS16550_plat *)dev->platform_data;
	int width = dev->resource[0].flags & IORESOURCE_MEM_TYPE_MASK;

	off <<= plat->shift;

	if (plat->reg_write) {
		plat->reg_write(val, (unsigned long)dev->priv, off);
		return;
	}

	switch (width) {
	case IORESOURCE_MEM_8BIT:
		writeb(val & 0xff, dev->priv + off);
		break;
	case IORESOURCE_MEM_16BIT:
		writew(val & 0xffff, dev->priv + off);
		break;
	case IORESOURCE_MEM_32BIT:
		writel(val, dev->priv + off);
		break;
	}
}

/**
 * @brief Compute the divisor for a baud rate
 *
 * @param[in] cdev pointer to console device
 * @param[in] baudrate baud rate
 *
 * @return divisor to be set
 */
static inline unsigned int ns16550_calc_divisor(struct console_device *cdev,
					 unsigned int baudrate)
{
	struct NS16550_plat *plat = (struct NS16550_plat *)
	    cdev->dev->platform_data;
	unsigned int clk = plat->clock;

	return (clk / MODE_X_DIV / baudrate);

}

/**
 * @brief Initialize the device
 *
 * @param[in] cdev pointer to console device
 */
static void ns16550_serial_init_port(struct console_device *cdev)
{
	/* initializing the device for the first time */
	ns16550_write(cdev, 0x00, lcr); /* select ier reg */
	ns16550_write(cdev, 0x00, ier);

#ifdef CONFIG_DRIVER_SERIAL_NS16550_OMAP_EXTENSIONS
	ns16550_write(cdev, 0x07, mdr1);	/* Disable */
#endif

	ns16550_setbaudrate(cdev, CONFIG_BAUDRATE);

#ifdef CONFIG_DRIVER_SERIAL_NS16550_OMAP_EXTENSIONS
	ns16550_write(cdev, 0x00,  mdr1);
#endif
}

/*********** Exposed Functions **********************************/

/**
 * @brief Put a character to the serial port
 *
 * @param[in] cdev pointer to console device
 * @param[in] c character to put
 */
static void ns16550_putc(struct console_device *cdev, char c)
{
	/* Loop Doing Nothing */
	while ((ns16550_read(cdev, lsr) & LSR_THRE) == 0) ;
	ns16550_write(cdev, c, thr);
}

/**
 * @brief Retrieve a character from serial port
 *
 * @param[in] cdev pointer to console device
 *
 * @return return the character read
 */
static int ns16550_getc(struct console_device *cdev)
{
	/* Loop Doing Nothing */
	while ((ns16550_read(cdev, lsr) & LSR_DR) == 0) ;
	return ns16550_read(cdev, rbr);
}

/**
 * @brief Test if character is available
 *
 * @param[in] cdev pointer to console device
 *
 * @return  - status based on data availability
 */
static int ns16550_tstc(struct console_device *cdev)
{
	return ((ns16550_read(cdev, lsr) & LSR_DR) != 0);
}

/**
 * @brief Set the baudrate for the uart port
 *
 * @param[in] cdev  console device
 * @param[in] baud_rate baud rate to set
 *
 * @return  0-implied to support the baudrate
 */
static int ns16550_setbaudrate(struct console_device *cdev, int baud_rate)
{
	unsigned int baud_divisor = ns16550_calc_divisor(cdev, baud_rate);
	struct NS16550_plat *plat = (struct NS16550_plat *)
	    cdev->dev->platform_data;

	ns16550_write(cdev, LCR_BKSE, lcr);
	ns16550_write(cdev, baud_divisor & 0xff, dll);
	ns16550_write(cdev, (baud_divisor >> 8) & 0xff, dlm);
	ns16550_write(cdev, LCRVAL, lcr);
	ns16550_write(cdev, MCRVAL, mcr);

	if (plat->flags & NS16650_FLAG_DISABLE_FIFO)
		ns16550_write(cdev, FCRVAL & ~FCR_FIFO_EN, fcr);
	else
		ns16550_write(cdev, FCRVAL, fcr);

	return 0;
}

/**
 * @brief Probe entry point -called on the first match for device
 *
 * @param[in] dev matched device
 *
 * @return EINVAL if platform_data is not populated,
 *	   ENOMEM if calloc failed
 *	   else return result of console_register
 */
static int ns16550_probe(struct device_d *dev)
{
	struct console_device *cdev;
	struct NS16550_plat *plat = (struct NS16550_plat *)dev->platform_data;

	/* we do expect platform specific data */
	if (plat == NULL)
		return -EINVAL;
	dev->priv = dev_request_mem_region(dev, 0);

	cdev = xzalloc(sizeof(*cdev));

	cdev->dev = dev;
	if (plat->f_caps)
		cdev->f_caps = plat->f_caps;
	else
		cdev->f_caps = CONSOLE_STDIN | CONSOLE_STDOUT | CONSOLE_STDERR;
	cdev->tstc = ns16550_tstc;
	cdev->putc = ns16550_putc;
	cdev->getc = ns16550_getc;
	cdev->setbrg = ns16550_setbaudrate;

	ns16550_serial_init_port(cdev);

	return console_register(cdev);
}

/**
 * @brief Driver registration structure
 */
static struct driver_d ns16550_serial_driver = {
	.name = "ns16550_serial",
	.probe = ns16550_probe,
};
console_platform_driver(ns16550_serial_driver);
