#include <platform_data/dw_mmc.h>
#include <bootsource.h>
#include <bootstrap.h>
#include <ns16550.h>
#include <common.h>
#include <malloc.h>
#include <init.h>
#include <envfs.h>
#include <sizes.h>
#include <fs.h>
#include <io.h>

#include <linux/clkdev.h>
#include <linux/stat.h>
#include <linux/clk.h>

#include <mach/system-manager.h>
#include <mach/socfpga-regs.h>

enum socfpga_clks {
	timer, mmc, uart, clk_max
};

static struct clk *clks[clk_max];

static struct dw_mmc_platform_data mmc_pdata = {
	.ciu_div = 3,
};

static void socfpga_mmc_init(void)
{
	clks[mmc] = clk_fixed("mmc", 400000000);
	clkdev_add_physbase(clks[mmc], CYCLONE5_SDMMC_ADDRESS, NULL);
	add_generic_device("dw_mmc", 0, NULL, CYCLONE5_SDMMC_ADDRESS, SZ_4K,
			IORESOURCE_MEM, &mmc_pdata);
}

static struct NS16550_plat uart_pdata = {
	.clock = 100000000,
	.shift = 2,
};

static void socfpga_uart_init(void)
{
	clks[uart] = clk_fixed("uart", 100000000);
	clkdev_add_physbase(clks[uart], CYCLONE5_UART0_ADDRESS, NULL);
	clkdev_add_physbase(clks[uart], CYCLONE5_UART1_ADDRESS, NULL);
	add_ns16550_device(0, 0xffc02000, 1024, IORESOURCE_MEM_8BIT,
			&uart_pdata);
}

static void socfpga_timer_init(void)
{
	clks[timer] = clk_fixed("timer", 200000000);
	clkdev_add_physbase(clks[timer], CYCLONE5_SMP_TWD_ADDRESS, NULL);
	add_generic_device("smp_twd", 0, NULL, CYCLONE5_SMP_TWD_ADDRESS, 0x100,
			IORESOURCE_MEM, NULL);
}

static __noreturn int socfpga_xload(void)
{
	enum bootsource bootsource = bootsource_get();
	void *buf;

	switch (bootsource) {
	case BOOTSOURCE_MMC:
		buf = bootstrap_read_disk("disk0.1", "fat");
		break;
	default:
		pr_err("unknown bootsource %d\n", bootsource);
		hang();
	}

	if (!buf) {
		pr_err("failed to load barebox.bin\n");
		hang();
	}

	pr_info("starting bootloader...\n");

	bootstrap_boot(buf, 0);

	hang();
}

static int socfpga_devices_init(void)
{
	barebox_set_model("SoCFPGA");
	socfpga_timer_init();
	socfpga_uart_init();
	socfpga_mmc_init();

	barebox_main = socfpga_xload;

	return 0;
}
coredevice_initcall(socfpga_devices_init);
