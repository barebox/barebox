/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <init.h>
#include <malloc.h>
#include <errno.h>
#include <mach/omap4_rom_usb.h>

struct serial_omap4_usbboot_priv {
	struct console_device cdev;
	u32 val;
};

static void serial_omap4_usbboot_putc(struct console_device *cdev, char c)
{
	unsigned b = c;
	omap4_usbboot_write(&b, 4);
}

static int serial_omap4_usbboot_tstc(struct console_device *cdev)
{
	struct serial_omap4_usbboot_priv *priv =
		container_of(cdev, struct serial_omap4_usbboot_priv, cdev);
	if (omap4_usbboot_is_read_waiting())
		return 0;
	else if (omap4_usbboot_is_read_ok())
		return 1;
	omap4_usbboot_queue_read(&priv->val, 4);
	udelay(100);
	if (omap4_usbboot_is_read_waiting())
		return 0;
	else if (omap4_usbboot_is_read_ok())
		return 1;
	return 0;
}

static int serial_omap4_usbboot_getc(struct console_device *cdev)
{
	struct serial_omap4_usbboot_priv *priv =
		container_of(cdev, struct serial_omap4_usbboot_priv, cdev);
	if (omap4_usbboot_is_read_waiting() || omap4_usbboot_is_read_ok()) {
		omap4_usbboot_wait_read();
		return priv->val;
	}
	omap4_usbboot_read(&priv->val, 4);
	return priv->val;
}

static int serial_omap4_usbboot_probe(struct device_d *dev)
{
	struct serial_omap4_usbboot_priv *priv;
	priv = xzalloc(sizeof(*priv));

	priv->cdev.dev = dev;
	priv->cdev.tstc = serial_omap4_usbboot_tstc;
	priv->cdev.putc = serial_omap4_usbboot_putc;
	priv->cdev.getc = serial_omap4_usbboot_getc;
	priv->cdev.setbrg = NULL;

	return console_register(&priv->cdev);
}

static struct driver_d serial_omap4_usbboot_driver = {
	.name = "serial_omap4_usbboot",
	.probe = serial_omap4_usbboot_probe,
};
console_platform_driver(serial_omap4_usbboot_driver);
