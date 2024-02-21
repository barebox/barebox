// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <init.h>
#include <envfs.h>
#include <bbu.h>
#include <bootsource.h>
#include <asm/memory.h>
#include <linux/sizes.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <soc/fsl/immap_lsch2.h>
#include <mach/layerscape/bbu.h>
#include <mach/layerscape/layerscape.h>

static int tqmls1046a_mem_init(void)
{
	int ret;

	if (!of_machine_is_compatible("tq,ls1046a-tqmls1046a"))
		return 0;

	arm_add_mem_device("ram0", 0x80000000, SZ_2G);

	ret = ls1046a_ppa_init(0x100000000 - SZ_64M, SZ_64M);
	if (ret)
		pr_err("Failed to initialize PPA firmware: %s\n", strerror(-ret));

        return 0;
}
mem_initcall(tqmls1046a_mem_init);

static int tqmls1046a_postcore_init(void)
{
	struct ccsr_scfg *scfg = IOMEM(LSCH2_SCFG_ADDR);
	enum bootsource bootsource;
	unsigned long sd_bbu_flags = 0, qspi_bbu_flags = 0;

	if (!of_machine_is_compatible("tq,ls1046a-tqmls1046a"))
		return 0;

	defaultenv_append_directory(defaultenv_tqmls1046a);

	/* Configure iomux for i2c4 */
	out_be32(&scfg->rcwpmuxcr0, 0x3300);

	/* divide CGA1/CGA2 PLL by 24 to get QSPI interface clock */
	out_be32(&scfg->qspi_cfg, 0x30100000);

	bootsource = ls1046a_bootsource_get();

	switch (bootsource) {
	case BOOTSOURCE_MMC:
		of_device_enable_path("/chosen/environment-sd");
		sd_bbu_flags = BBU_HANDLER_FLAG_DEFAULT;
		break;
	case BOOTSOURCE_SPI_NOR:
		of_device_enable_path("/chosen/environment-qspi");
		qspi_bbu_flags = BBU_HANDLER_FLAG_DEFAULT;
		break;
	default:
		break;
	}

	ls1046a_bbu_mmc_register_handler("sd", "/dev/mmc0.barebox", sd_bbu_flags);
	ls1046a_bbu_qspi_register_handler("qspi", "/dev/qspiflash0.barebox", qspi_bbu_flags);

	return 0;
}

postcore_initcall(tqmls1046a_postcore_init);
