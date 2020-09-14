#include <common.h>
#include <init.h>
#include <poweroff.h>
#include <restart.h>
#include <mach/linux.h>

static void sandbox_poweroff(struct poweroff_handler *poweroff)
{
	linux_exit();
}

static void sandbox_rst_hang(struct restart_handler *rst)
{
	linux_hang();
}

static struct restart_handler rst_hang = {
	.name = "hang",
	.restart = sandbox_rst_hang
};

static int poweroff_register_feature(void)
{
	poweroff_handler_register_fn(sandbox_poweroff);
	restart_handler_register(&rst_hang);

	return 0;
}
coredevice_initcall(poweroff_register_feature);
