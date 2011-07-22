#include <common.h>
#include <init.h>
#include <driver.h>
#include <asm/cpu/cdefBF561.h>
#include <partition.h>
#include <fs.h>

static struct device_d smc911x_dev = {
	.id	  = -1,
	.name     = "smc911x",
	.map_base = 0x24000000,
	.size     = 4096,
};

static int ipe337_devices_init(void) {
	add_cfi_flash_device(-1, 0x20000000, 32 * 1024 * 1024, 0);
	add_mem_device("ram0", 0x0, 128 * 1024 * 1024,
		       IORESOURCE_MEM_WRITEABLE);

	/* Reset smc911x */
	*pFIO0_DIR = (1<<12);
	*pFIO0_FLAG_C = (1<<12);
	mdelay(100);
	*pFIO0_FLAG_S = (1<<12);

	register_device(&smc911x_dev);

	devfs_add_partition("nor0", 0x00000, 0x20000, PARTITION_FIXED, "self0");
	devfs_add_partition("nor0", 0x20000, 0x20000, PARTITION_FIXED, "env0");

	protect_file("/dev/env0", 1);

	return 0;
}

device_initcall(ipe337_devices_init);

static struct device_d blackfin_serial_device = {
	.id	  = -1,
	.name     = "blackfin_serial",
	.map_base = 0,
	.size     = 4096,
};

static int blackfin_console_init(void)
{
	register_device(&blackfin_serial_device);

	return 0;
}

console_initcall(blackfin_console_init);

