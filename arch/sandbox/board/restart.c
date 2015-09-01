#include <common.h>
#include <init.h>
#include <restart.h>
#include <mach/linux.h>

static void sandbox_restart_cpu(struct restart_handler *restart)
{
	linux_exit();
}

static int restart_register_feature(void)
{
	restart_handler_register_fn(sandbox_restart_cpu);

	return 0;
}
coredevice_initcall(restart_register_feature);
