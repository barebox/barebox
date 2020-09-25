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

static void sandbox_rst_reexec(struct restart_handler *rst)
{
	linux_reexec();
}

static struct restart_handler rst_reexec = {
	.name = "reexec", .priority = 200,
	.restart = sandbox_rst_reexec,
};

static int poweroff_register_feature(void)
{
	poweroff_handler_register_fn(sandbox_poweroff);
	restart_handler_register(&rst_hang);

	if (IS_ENABLED(CONFIG_SANDBOX_REEXEC))
		restart_handler_register(&rst_reexec);

	return 0;
}
coredevice_initcall(poweroff_register_feature);
