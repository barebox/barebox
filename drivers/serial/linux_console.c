#include <common.h>
#include <driver.h>
#include <init.h>
#include <asm/arch/linux.h>
#include <malloc.h>
#include <console.h>

static void linux_console_putc(struct console_device *cdev, char c)
{
	linux_putc(c);
}

static int linux_console_tstc(struct console_device *cdev)
{
       return linux_tstc();
}

static int linux_console_getc(struct console_device *cdev)
{
	return linux_getc();
}

static int linux_console_probe(struct device_d *dev)
{
	struct console_device *cdev;

	cdev = malloc(sizeof(struct console_device));
	dev->type_data = cdev;
	cdev->dev = dev;
	cdev->flags = CONSOLE_STDIN | CONSOLE_STDOUT | CONSOLE_STDERR;
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

static struct device_d linux_console_device = {
        .name  = "console",
        .id    = "cs0",
        .type  = DEVICE_TYPE_CONSOLE,
};

#if 0
static struct device_d linux_console_device1 = {
        .name  = "console",
        .id    = "cs1",
        .type  = DEVICE_TYPE_CONSOLE,
};
#endif

static int console_init(void)
{
	register_driver(&linux_console_driver);
	register_device(&linux_console_device);
//	register_device(&linux_console_device1);
	return 0;
}

console_initcall(console_init);

