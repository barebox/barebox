/*
 * Copyright (C) 2013 Sascha Hauer <s.hauer@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
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
#include <mach/imx6-mmdc.h>
#include <mach/imx6.h>

static inline void setup_uart(void)
{
	void __iomem *ccmbase = (void *)MX6_CCM_BASE_ADDR;
	void __iomem *uartbase = (void *)MX6_UART4_BASE_ADDR;
	void __iomem *iomuxbase = (void *)MX6_IOMUXC_BASE_ADDR;

	writel(0x4, iomuxbase + 0x01f8);

	writel(0xffffffff, ccmbase + 0x68);
	writel(0xffffffff, ccmbase + 0x6c);
	writel(0xffffffff, ccmbase + 0x70);
	writel(0xffffffff, ccmbase + 0x74);
	writel(0xffffffff, ccmbase + 0x78);
	writel(0xffffffff, ccmbase + 0x7c);
	writel(0xffffffff, ccmbase + 0x80);

	writel(0x00000000, uartbase + 0x80);
	writel(0x00004027, uartbase + 0x84);
	writel(0x00000704, uartbase + 0x88);
	writel(0x00000a81, uartbase + 0x90);
	writel(0x0000002b, uartbase + 0x9c);
	writel(0x00013880, uartbase + 0xb0);
	writel(0x0000047f, uartbase + 0xa4);
	writel(0x0000c34f, uartbase + 0xa8);
	writel(0x00000001, uartbase + 0x80);

	putc_ll('>');
}

#define SZ_4G 0xEFFFFFF8

BAREBOX_IMD_TAG_STRING(phyflex_mx6_memsize_SZ_512M, IMD_TYPE_PARAMETER, "memsize=512", 0);
BAREBOX_IMD_TAG_STRING(phyflex_mx6_memsize_SZ_1G, IMD_TYPE_PARAMETER, "memsize=1024", 0);
BAREBOX_IMD_TAG_STRING(phyflex_mx6_memsize_SZ_2G, IMD_TYPE_PARAMETER, "memsize=2048", 0);
BAREBOX_IMD_TAG_STRING(phyflex_mx6_memsize_SZ_4G, IMD_TYPE_PARAMETER, "memsize=4096", 0);

static void __noreturn start_imx6_phytec_common(uint32_t size,
						bool do_early_uart_config,
						void *fdt_blob_fixed_offset)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	arm_setup_stack(0x00920000 - 8);

	if (do_early_uart_config && IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	fdt = fdt_blob_fixed_offset - get_runtime_offset();
	barebox_arm_entry(0x10000000, size, fdt);
}

#define PHYTEC_ENTRY(name, fdt_name, memory_size, do_early_uart_config)	\
	ENTRY_FUNCTION(name, r0, r1, r2)				\
	{								\
		extern char __dtb_##fdt_name##_start[];			\
									\
		IMD_USED(phyflex_mx6_memsize_##memory_size);		\
									\
		start_imx6_phytec_common(memory_size, do_early_uart_config, \
					 __dtb_##fdt_name##_start);	\
	}

PHYTEC_ENTRY(start_phytec_pbab01_1gib, imx6q_phytec_pbab01, SZ_1G, true);
PHYTEC_ENTRY(start_phytec_pbab01_2gib, imx6q_phytec_pbab01, SZ_2G, true);
PHYTEC_ENTRY(start_phytec_pbab01_4gib, imx6q_phytec_pbab01, SZ_4G, true);
PHYTEC_ENTRY(start_phytec_pbab01dl_1gib, imx6dl_phytec_pbab01, SZ_1G, false);
PHYTEC_ENTRY(start_phytec_pbab01s_512mb, imx6s_phytec_pbab01, SZ_512M, false);
PHYTEC_ENTRY(start_phytec_phyboard_alcor_1gib, imx6q_phytec_phyboard_alcor, SZ_1G, false);
PHYTEC_ENTRY(start_phytec_phyboard_subra_512mb, imx6dl_phytec_phyboard_subra, SZ_512M, false);
