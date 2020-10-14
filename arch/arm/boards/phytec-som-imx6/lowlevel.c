// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2013 Sascha Hauer <s.hauer@pengutronix.de>
// SPDX-FileCopyrightText: 2015 PHYTEC Messtechnik GmbH

/*
 * Author: Stefan Christ <s.christ@phytec.de>
 */
#include <debug_ll.h>
#include <common.h>
#include <linux/sizes.h>
#include <io.h>
#include <image-metadata.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <asm/sections.h>
#include <asm/cache.h>
#include <asm/mmu.h>
#include <mach/imx6.h>

static inline void setup_uart(void)
{
	void __iomem *iomuxbase = (void *)MX6_IOMUXC_BASE_ADDR;

	writel(0x4, iomuxbase + 0x01f8);

	imx6_ungate_all_peripherals();
	imx6_uart_setup_ll();

	putc_ll('>');
}

#undef SZ_4G
#define SZ_4G 0xEFFFFFF8

BAREBOX_IMD_TAG_STRING(physom_mx6_memsize_SZ_128M, IMD_TYPE_PARAMETER, "memsize=128", 0);
BAREBOX_IMD_TAG_STRING(physom_mx6_memsize_SZ_256M, IMD_TYPE_PARAMETER, "memsize=256", 0);
BAREBOX_IMD_TAG_STRING(physom_mx6_memsize_SZ_512M, IMD_TYPE_PARAMETER, "memsize=512", 0);
BAREBOX_IMD_TAG_STRING(physom_mx6_memsize_SZ_1G, IMD_TYPE_PARAMETER, "memsize=1024", 0);
BAREBOX_IMD_TAG_STRING(physom_mx6_memsize_SZ_2G, IMD_TYPE_PARAMETER, "memsize=2048", 0);
BAREBOX_IMD_TAG_STRING(physom_mx6_memsize_SZ_4G, IMD_TYPE_PARAMETER, "memsize=4096", 0);

static void __noreturn start_imx6_phytec_common(uint32_t size,
						bool do_early_uart_config,
						void *fdt_blob_fixed_offset)
{
	int cpu_type = __imx6_cpu_type();
	void *fdt;

	if (cpu_type == IMX6_CPUTYPE_IMX6UL
	    || cpu_type == IMX6_CPUTYPE_IMX6ULL) {
		imx6ul_cpu_lowlevel_init();
		/* OCRAM Free Area is 0x00907000 to 0x00918000 (68KB) */
		arm_setup_stack(0x00910000);
	} else {
		imx6_cpu_lowlevel_init();
		/* OCRAM Free Area is 0x00907000 to 0x00938000 (196KB) */
		arm_setup_stack(0x00920000);
	}

	if (do_early_uart_config && IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	fdt = fdt_blob_fixed_offset + get_runtime_offset();

	if (cpu_type == IMX6_CPUTYPE_IMX6UL
	    || cpu_type == IMX6_CPUTYPE_IMX6ULL)
		barebox_arm_entry(0x80000000, size, fdt);
	else
		barebox_arm_entry(0x10000000, size, fdt);
}

#define PHYTEC_ENTRY(name, fdt_name, memory_size, do_early_uart_config)	\
	ENTRY_FUNCTION(name, r0, r1, r2)				\
	{								\
		extern char __dtb_##fdt_name##_start[];			\
									\
		IMD_USED(physom_mx6_memsize_##memory_size);		\
		IMD_USED_OF(fdt_name);					\
									\
		start_imx6_phytec_common(memory_size, do_early_uart_config, \
					 __dtb_##fdt_name##_start);	\
	}

PHYTEC_ENTRY(start_phytec_phycard_imx6q_1gib, imx6q_phytec_phycard, SZ_1G, true);
PHYTEC_ENTRY(start_phytec_phycard_imx6q_1gib_1bank, imx6q_phytec_phycard, SZ_1G, true);
PHYTEC_ENTRY(start_phytec_phycard_imx6q_2gib, imx6q_phytec_phycard, SZ_2G, true);

PHYTEC_ENTRY(start_phytec_pbab01_512mb_1bank, imx6q_phytec_pbab01, SZ_512M, true);
PHYTEC_ENTRY(start_phytec_pbab01_1gib, imx6q_phytec_pbab01, SZ_1G, true);
PHYTEC_ENTRY(start_phytec_pbab01_1gib_1bank, imx6q_phytec_pbab01, SZ_1G, true);
PHYTEC_ENTRY(start_phytec_pbab01_2gib, imx6q_phytec_pbab01, SZ_2G, true);
PHYTEC_ENTRY(start_phytec_pbab01_4gib, imx6q_phytec_pbab01, SZ_4G, true);
PHYTEC_ENTRY(start_phytec_pbab01dl_1gib, imx6dl_phytec_pbab01, SZ_1G, false);
PHYTEC_ENTRY(start_phytec_pbab01dl_1gib_1bank, imx6dl_phytec_pbab01, SZ_1G, false);
PHYTEC_ENTRY(start_phytec_pbab01s_128mb_1bank, imx6s_phytec_pbab01, SZ_128M, false);
PHYTEC_ENTRY(start_phytec_pbab01s_256mb_1bank, imx6s_phytec_pbab01, SZ_256M, false);
PHYTEC_ENTRY(start_phytec_pbab01s_512mb_1bank, imx6s_phytec_pbab01, SZ_512M, false);
PHYTEC_ENTRY(start_phytec_phyboard_alcor_1gib, imx6q_phytec_phyboard_alcor, SZ_1G, false);
PHYTEC_ENTRY(start_phytec_phyboard_subra_512mb_1bank, imx6dl_phytec_phyboard_subra, SZ_512M, false);
PHYTEC_ENTRY(start_phytec_phyboard_subra_1gib_1bank, imx6q_phytec_phyboard_subra, SZ_1G, false);

PHYTEC_ENTRY(start_phytec_phycore_imx6dl_som_nand_256mb, imx6dl_phytec_phycore_som_nand, SZ_256M, true);
PHYTEC_ENTRY(start_phytec_phycore_imx6dl_som_lc_nand_256mb, imx6dl_phytec_phycore_som_lc_nand, SZ_256M, true);
PHYTEC_ENTRY(start_phytec_phycore_imx6dl_som_nand_1gib, imx6dl_phytec_phycore_som_nand, SZ_1G, true);
PHYTEC_ENTRY(start_phytec_phycore_imx6dl_som_emmc_512mb, imx6dl_phytec_phycore_som_emmc, SZ_512M, true);
PHYTEC_ENTRY(start_phytec_phycore_imx6dl_som_emmc_1gib, imx6dl_phytec_phycore_som_emmc, SZ_1G, true);
PHYTEC_ENTRY(start_phytec_phycore_imx6dl_som_lc_emmc_1gib, imx6dl_phytec_phycore_som_lc_emmc, SZ_1G, true);
PHYTEC_ENTRY(start_phytec_phycore_imx6q_som_nand_1gib, imx6q_phytec_phycore_som_nand, SZ_1G, true);
PHYTEC_ENTRY(start_phytec_phycore_imx6qp_som_nand_1gib, imx6qp_phytec_phycore_som_nand, SZ_1G, true);
PHYTEC_ENTRY(start_phytec_phycore_imx6q_som_emmc_1gib, imx6q_phytec_phycore_som_emmc, SZ_1G, true);
PHYTEC_ENTRY(start_phytec_phycore_imx6q_som_emmc_2gib, imx6q_phytec_phycore_som_emmc, SZ_2G, true);

PHYTEC_ENTRY(start_phytec_phycore_imx6ul_som_emmc_512mb, imx6ul_phytec_phycore_som_emmc, SZ_512M, false);
PHYTEC_ENTRY(start_phytec_phycore_imx6ul_som_nand_512mb, imx6ul_phytec_phycore_som_nand, SZ_512M, false);
PHYTEC_ENTRY(start_phytec_phycore_imx6ull_som_lc_nand_256mb, imx6ull_phytec_phycore_som_lc_nand, SZ_256M, false);
PHYTEC_ENTRY(start_phytec_phycore_imx6ull_som_nand_512mb, imx6ull_phytec_phycore_som_nand, SZ_512M, false);
PHYTEC_ENTRY(start_phytec_phycore_imx6ull_som_emmc_512mb, imx6ull_phytec_phycore_som_emmc, SZ_512M, false);
