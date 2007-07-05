#include <common.h>
#include <driver.h>
#include <init.h>
#include <asm/arch/linux.h>
#include <malloc.h>
#include <console.h>

static void linux_console_putc(struct console_device *cdev, char c)
{
	struct device_d *dev = cdev->dev;
	struct linux_console_data *d = dev->platform_data;

	linux_write(d->stdoutfd, &c, 1);
}

static int linux_console_tstc(struct console_device *cdev)
{
	struct device_d *dev = cdev->dev;
	struct linux_console_data *d = dev->platform_data;

	return linux_tstc(d->stdinfd);
}

static int linux_console_getc(struct console_device *cdev)
{
	struct device_d *dev = cdev->dev;
	struct linux_console_data *d = dev->platform_data;
	char c;

	linux_read(d->stdinfd, &c, 1);

	return c;
}

static int linux_console_probe(struct device_d *dev)
{
	struct console_device *cdev;
	struct linux_console_data *data = dev->platform_data;

	cdev = malloc(sizeof(struct console_device));
	dev->type_data = cdev;
	cdev->dev = dev;
	cdev->f_caps = data->flags;
	cdev->tstc = linux_console_tstc;
	cdev->putc = linux_console_putc;
	cdev->getc = linux_console_getc;

	console_register(cdev);

	return 0;
}

static struct driver_d linux_console_driver = {
        .name  = "console",
        .probe = linux_console_probe,
        .type  = DEVICE_TYPE_CONSOLE,
};

static int console_init(void)
{
	return register_driver(&linux_console_driver);
}

console_initcall(console_init);

