// SPDX-License-Identifier: GPL-2.0-or-later

#include <common.h>
#include <init.h>
#include <malloc.h>
#include <errno.h>
#include <mach/omap/omap4_rom_usb.h>

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

static int serial_omap4_usbboot_probe(struct device *dev)
{
	struct serial_omap4_usbboot_priv *priv;
	int ret;

	ret = omap4_usbboot_open();
	if (ret)
		return ret;

	priv = xzalloc(sizeof(*priv));

	priv->cdev.dev = dev;
	priv->cdev.tstc = serial_omap4_usbboot_tstc;
	priv->cdev.putc = serial_omap4_usbboot_putc;
	priv->cdev.getc = serial_omap4_usbboot_getc;
	priv->cdev.setbrg = NULL;

	return console_register(&priv->cdev);
}

static struct driver serial_omap4_usbboot_driver = {
	.name = "serial_omap4_usbboot",
	.probe = serial_omap4_usbboot_probe,
};
console_platform_driver(serial_omap4_usbboot_driver);
