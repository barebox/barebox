#include <common.h>
#include <command.h>
#include <init.h>
#include <driver.h>
#include <malloc.h>
#include <errno.h>

static struct device_d global_dev;

int global_add_parameter(struct param_d *param)
{
        return dev_add_parameter(&global_dev, param);
}

static struct device_d global_dev = {
        .name  = "global",
	.id    = "env",
        .map_base = 0,
        .size   = 0,
};

static struct driver_d global_driver = {
        .name  = "global",
        .probe = dummy_probe,
};

static int global_init(void)
{
	register_device(&global_dev);
        register_driver(&global_driver);
        return 0;
}

coredevice_initcall(global_init);

