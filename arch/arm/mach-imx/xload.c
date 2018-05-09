#include <bootsource.h>
#include <bootstrap.h>
#include <common.h>
#include <malloc.h>
#include <init.h>
#include <envfs.h>
#include <linux/sizes.h>
#include <fs.h>
#include <io.h>

#include <linux/clkdev.h>
#include <linux/stat.h>
#include <linux/clk.h>

#include <mach/devices-imx51.h>

static __noreturn int imx_xload(void)
{
	enum bootsource bootsource = bootsource_get();
	void *buf;

	switch (bootsource) {
	case BOOTSOURCE_MMC:
		pr_info("booting from MMC\n");
		buf = bootstrap_read_disk("disk0.0", "fat");
		break;
	case BOOTSOURCE_SPI_NOR:
		pr_info("booting from SPI\n");
		buf = bootstrap_read_devfs("dataflash0", false,
					   SZ_256K, SZ_1M, SZ_1M);
		break;
	default:
		pr_err("unknown bootsource %d\n", bootsource);
		hang();
	}

	if (!buf) {
		pr_err("failed to load barebox.bin\n");
		hang();
	}

	bootstrap_boot(buf, 0);

	hang();
}

static int imx_devices_init(void)
{
	barebox_main = imx_xload;
	return 0;
}
coredevice_initcall(imx_devices_init);
