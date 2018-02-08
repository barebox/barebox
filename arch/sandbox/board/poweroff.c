#include <common.h>
#include <init.h>
#include <poweroff.h>
#include <mach/linux.h>

static void sandbox_poweroff(struct poweroff_handler *poweroff)
{
	linux_exit();
}

static int poweroff_register_feature(void)
{
	poweroff_handler_register_fn(sandbox_poweroff);

	return 0;
}
coredevice_initcall(poweroff_register_feature);
