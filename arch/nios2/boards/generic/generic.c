#include <common.h>
#include <init.h>
#include <driver.h>
#include <partition.h>
#include <fs.h>

static struct device_d cfi_dev = {
	.id       = -1,
	.name     = "cfi_flash",
	.map_base = NIOS_SOPC_FLASH_BASE,
	.size     = NIOS_SOPC_FLASH_SIZE,
};

static int phy_address = 1;

static struct device_d mac_dev = {
	.id            = -1,
	.name          = "altera_tse",
	.map_base      = NIOS_SOPC_TSE_BASE,
	.size          = 0x00000400,
	.platform_data = &phy_address,
};

static struct memory_platform_data ram_pdata = {
	.name  = "ram0",
	.flags = DEVFS_RDWR,
};

static struct device_d ram_dev = {
	.id            = -1,
	.name          = "mem",
	.map_base      = NIOS_SOPC_MEMORY_BASE,
	.size          = NIOS_SOPC_MEMORY_SIZE,
	.platform_data = &ram_pdata,
};

static struct device_d altera_serial_device = {
	.id       = -1,
	.name     = "altera_serial",
	.map_base = NIOS_SOPC_UART_BASE,
};

/*
static struct device_d epcs_flash_device = {
	.id       = -1,
	.name     = "epcs_flash",
	.map_base = NIOS_SOPC_EPCS_BASE,
};
*/

static int generic_devices_init(void)
{
	register_device(&cfi_dev);
	register_device(&ram_dev);
	register_device(&mac_dev);
	/*register_device(&epcs_flash_device);*/

	devfs_add_partition("nor0", 0x00000, 0x40000, PARTITION_FIXED, "self0");
	devfs_add_partition("nor0", 0x40000, 0x20000, PARTITION_FIXED, "env0");

	protect_file("/dev/env0", 1);

	return 0;
}

device_initcall(generic_devices_init);


static int altera_console_init(void)
{
	register_device(&altera_serial_device);

	return 0;
}

console_initcall(altera_console_init);

