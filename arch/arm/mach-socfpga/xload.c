#include <platform_data/cadence_qspi.h>
#include <platform_data/dw_mmc.h>
#include <bootsource.h>
#include <bootstrap.h>
#include <platform_data/serial-ns16550.h>
#include <common.h>
#include <malloc.h>
#include <init.h>
#include <envfs.h>
#include <linux/sizes.h>
#include <fs.h>
#include <io.h>
#include <mci.h>

#include <linux/clkdev.h>
#include <linux/stat.h>
#include <linux/clk.h>

#include <mach/generic.h>
#include <mach/system-manager.h>
#include <mach/socfpga-regs.h>

static struct socfpga_barebox_part default_parts[] = {
	{
		.nor_offset = SZ_256K,
		.nor_size = SZ_1M,
		.mmc_disk = "disk0.1",
	},
	{ /* sentinel */ }
};
const struct socfpga_barebox_part *barebox_parts = &default_parts;

enum socfpga_clks {
	timer, mmc, qspi_clk, uart, clk_max
};

static struct clk *clks[clk_max];

static struct dw_mmc_platform_data mmc_pdata = {
	.bus_width_caps = MMC_CAP_4_BIT_DATA,
	.ciu_div = 3,
};

static void socfpga_mmc_init(void)
{
	clks[mmc] = clk_fixed("mmc", 400000000);
	clkdev_add_physbase(clks[mmc], CYCLONE5_SDMMC_ADDRESS, NULL);
	add_generic_device("dw_mmc", 0, NULL, CYCLONE5_SDMMC_ADDRESS, SZ_4K,
			IORESOURCE_MEM, &mmc_pdata);
}

#if defined(CONFIG_SPI_CADENCE_QUADSPI)
static struct cadence_qspi_platform_data qspi_pdata = {
	.ext_decoder = 0,
	.fifo_depth = 128,
};

static __maybe_unused void add_cadence_qspi_device(int id, resource_size_t ctrl,
				    resource_size_t data, void *pdata)
{
	struct resource *res;

	res = xzalloc(sizeof(struct resource) * 2);
	res[0].start = ctrl;
	res[0].end = ctrl + 0x100 - 1;
	res[0].flags = IORESOURCE_MEM;
	res[1].start = data;
	res[1].end = data + 0x100 - 1;
	res[1].flags = IORESOURCE_MEM;

	add_generic_device_res("cadence_qspi", id, res, 2, pdata);
}

static __maybe_unused void socfpga_qspi_init(void)
{
	clks[qspi_clk] = clk_fixed("qspi_clk", 370000000);
	clkdev_add_physbase(clks[qspi_clk], CYCLONE5_QSPI_CTRL_ADDRESS, NULL);
	clkdev_add_physbase(clks[qspi_clk], CYCLONE5_QSPI_DATA_ADDRESS, NULL);
	add_cadence_qspi_device(0, CYCLONE5_QSPI_CTRL_ADDRESS,
				CYCLONE5_QSPI_DATA_ADDRESS, &qspi_pdata);
}
#else
static void socfpga_qspi_init(void)
{
	return;
}
#endif

static struct NS16550_plat uart_pdata = {
	.clock = 100000000,
	.shift = 2,
};

static void socfpga_uart_init(void)
{
	clks[uart] = clk_fixed("uart", 100000000);
	clkdev_add_physbase(clks[uart], CYCLONE5_UART0_ADDRESS, NULL);
	clkdev_add_physbase(clks[uart], CYCLONE5_UART1_ADDRESS, NULL);
	add_ns16550_device(0, 0xffc02000, 1024, IORESOURCE_MEM |
			IORESOURCE_MEM_8BIT, &uart_pdata);
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
	struct socfpga_barebox_part *part;
	void *buf = NULL;

	switch (bootsource) {
	case BOOTSOURCE_MMC:
		socfpga_mmc_init();

		for (part = barebox_parts; part->mmc_disk; part++) {
			buf = bootstrap_read_disk(barebox_parts->mmc_disk, "fat");
			if (!buf) {
				pr_info("failed to load barebox from MMC %s\n",
					part->mmc_disk);
				continue;
			}
		}
		if (!buf) {
			pr_err("failed to load barebox.bin from MMC\n");
			hang();
		}
		break;
	case BOOTSOURCE_SPI:
		socfpga_qspi_init();

		for (part = barebox_parts; part->nor_size; part++) {
			buf = bootstrap_read_devfs("mtd0", false,
					part->nor_offset, part->nor_size, SZ_1M);
			if (!buf) {
				pr_info("failed to load barebox from QSPI NOR flash at offset %#x\n",
					part->nor_offset);
				continue;
			}

			break;
		}

		if (!buf) {
			pr_err("failed to load barebox from QSPI NOR flash\n");
			hang();
		}

		break;
	default:
		pr_err("unknown bootsource %d\n", bootsource);
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

	barebox_main = socfpga_xload;

	return 0;
}
coredevice_initcall(socfpga_devices_init);
