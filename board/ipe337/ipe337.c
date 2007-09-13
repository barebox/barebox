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

	.map_base = 0x08000000,
	.size     = 16 * 1024 * 1024,

	.type     = DEVICE_TYPE_DRAM,
};

static struct device_d smc911x_dev = {
	.name     = "smc911x",
	.id       = "eth0",
	.map_base = 0x24000000,
	.size     = 4096,
	.type     = DEVICE_TYPE_ETHER,
};

static int ipe337_devices_init(void) {
	register_device(&cfi_dev);
	register_device(&sdram_dev);

	*pFIO0_DIR = (1<<12);
	*pFIO0_FLAG_C = (1<<12);
	udelay(1000);
	*pFIO0_FLAG_S = (1<<12);
	udelay(1000);

	register_device(&smc911x_dev);

	dev_add_partition(&cfi_dev, 0x00000, 0x20000, "self");
	dev_add_partition(&cfi_dev, 0x40000, 0x20000, "env");
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

