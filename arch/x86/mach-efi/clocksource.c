// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <efi/efi-init.h>
#include <driver.h>

static int efi_x86_pure_init(void)
{
	struct device *dev = device_alloc("efi-cs-x86", DEVICE_ID_SINGLE);

	return platform_device_register(dev);
}
core_efi_initcall(efi_x86_pure_init);
