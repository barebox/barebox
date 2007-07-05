#include <common.h>
#include <driver.h>
#include <asm/arch/linux.h>
#include <xfuncs.h>

int u_boot_register_console(char *name_template, int stdinfd, int stdoutfd)
{
	struct device_d *dev;
	struct linux_console_data *data;

	dev = xzalloc(sizeof(struct device_d) + sizeof(struct linux_console_data));

	data = (struct linux_console_data *)(dev + 1);

	dev->platform_data = data;

	strcpy(dev->name,"console");


	if (stdinfd >= 0)
		data->flags = CONSOLE_STDIN;
	if (stdoutfd >= 0)
		data->flags |= CONSOLE_STDOUT | CONSOLE_STDERR;

	data->stdoutfd = stdoutfd;
	data->stdinfd  = stdinfd;

	get_free_deviceid(dev->id, name_template);
	dev->type = DEVICE_TYPE_CONSOLE;

	return register_device(dev);
}

