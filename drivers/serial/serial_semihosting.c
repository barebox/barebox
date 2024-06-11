// SPDX-License-Identifier: GPL-2.0+

#include <console.h>
#include <asm/semihosting.h>
#include <efi/efi-mode.h>

static void smh_serial_putc(struct console_device *cdev, char ch)
{
	semihosting_writec(ch);
}

static int smh_serial_probe(struct device *dev)
{
	struct console_device *cdev;

	cdev = xzalloc(sizeof(*cdev));

	cdev->dev = dev;
	cdev->putc = smh_serial_putc;

	return console_register(cdev);
}

static struct driver serial_semihosting_driver = {
	.name   = "serial_semihosting",
	.probe  = smh_serial_probe,
};

static int __init serial_semihosting_register(void)
{
	if (!efi_is_payload()) {
		struct device *dev;
		int ret;

		dev = device_alloc("serial_semihosting", 0);

		ret = platform_device_register(dev);
		if (ret)
			return ret;
	}

	return platform_driver_register(&serial_semihosting_driver);
}
console_initcall(serial_semihosting_register);
