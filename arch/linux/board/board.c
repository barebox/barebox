#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <asm/arch/linux.h>
#include <init.h>
#include <errno.h>
#include <asm-generic/errno.h>

static struct device_d tap_device = {
        .name     = "tap",
        .id       = "eth0",

        .type     = DEVICE_TYPE_ETHER,
};

static int devices_init(void)
{
	register_device(&tap_device);

	return 0;
}

device_initcall(devices_init);

