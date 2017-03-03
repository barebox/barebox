#include <common.h>
#include <init.h>
#include <driver.h>

static int efi_x86_pure_init(void)
{
	struct device_d *dev = device_alloc("efi-cs-x86", DEVICE_ID_SINGLE);

	return platform_device_register(dev);
}
core_initcall(efi_x86_pure_init);
