#include <common.h>
#include <init.h>
#include <driver.h>
#include <asm/cpu/cdefBF561.h>
#include <partition.h>

static struct device_d cfi_dev = {
	.name     = "cfi_flash",
	.id       = "nor0",

	.map_base = 0x20000000,
	.size     = 32 * 1024 * 1024,
};

static struct device_d sdram_dev = {
	.name     = "ram",
	.id       = "ram0",

	.map_base = 0x0,
	.size     = 128 * 1024 * 1024,

	.type     = DEVICE_TYPE_DRAM,
};

static struct device_d smc911x_dev = {
	.name     = "smc911x",
	.id       = "eth0",
	.map_base = 0x24000000,
	.size     = 4096,
};

static int ipe337_devices_init(void) {
	register_device(&cfi_dev);
	register_device(&sdram_dev);

	/* Reset smc911x */
	*pFIO0_DIR = (1<<12);
	*pFIO0_FLAG_C = (1<<12);
	mdelay(100);
	*pFIO0_FLAG_S = (1<<12);

	register_device(&smc911x_dev);

	devfs_add_partition("nor0", 0x00000, 0x20000, PARTITION_FIXED, "self");
	devfs_add_partition("nor0", 0x20000, 0x20000, PARTITION_FIXED, "env");

	dev_protect(&cfi_dev, 0x20000, 0, 1);

	return 0;
}

device_initcall(ipe337_devices_init);

static struct device_d blackfin_serial_device = {
	.name     = "blackfin_serial",
	.id       = "cs0",
	.map_base = 0,
	.size     = 4096,
	.type     = DEVICE_TYPE_CONSOLE,
};

static int blackfin_console_init(void)
{
	register_device(&blackfin_serial_device);

	return 0;
}

console_initcall(blackfin_console_init);

