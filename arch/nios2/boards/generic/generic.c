#include <common.h>
#include <init.h>
#include <driver.h>
#include <partition.h>
#include <fs.h>
#include <memory.h>
#include <envfs.h>

static int phy_address = 1;

static struct resource mac_resources[] = {
	[0] = {
		.start	= NIOS_SOPC_TSE_BASE,
		.end	= NIOS_SOPC_TSE_BASE + 0x400 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= NIOS_SOPC_SGDMA_RX_BASE,
		.end	= 0x40 + NIOS_SOPC_SGDMA_RX_BASE - 1,
		.flags	= IORESOURCE_MEM,
	},
	[2] = {
		.start	= NIOS_SOPC_SGDMA_TX_BASE,
		.end	= 0x40 + NIOS_SOPC_SGDMA_TX_BASE - 1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct device_d mac_dev = {
	.id            = DEVICE_ID_DYNAMIC,
	.name          = "altera_tse",
	.num_resources = ARRAY_SIZE(mac_resources),
	.resource      = mac_resources,
	.platform_data = &phy_address,
};

static int mem_init(void)
{
	barebox_add_memory_bank("ram0", NIOS_SOPC_MEMORY_BASE, NIOS_SOPC_MEMORY_SIZE);

	return 0;
}
mem_initcall(mem_init);

static int generic_devices_init(void)
{
	add_cfi_flash_device(DEVICE_ID_DYNAMIC, NIOS_SOPC_FLASH_BASE, NIOS_SOPC_FLASH_SIZE, 0);
	platform_device_register(&mac_dev);
	/*register_device(&epcs_flash_device);*/

	devfs_add_partition("nor0", 0x00000, 0x40000, DEVFS_PARTITION_FIXED, "self0");
	devfs_add_partition("nor0", 0x40000, 0x20000, DEVFS_PARTITION_FIXED, "env0");

	protect_file("/dev/env0", 1);

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_generic);

	return 0;
}

device_initcall(generic_devices_init);


static int altera_console_init(void)
{
	barebox_set_model("Altera Generic Board");
	barebox_set_hostname("nios2");

	add_generic_device("altera_serial", DEVICE_ID_DYNAMIC, NULL,
			NIOS_SOPC_UART_BASE, 0x20, IORESOURCE_MEM, NULL);

	return 0;
}

console_initcall(altera_console_init);

