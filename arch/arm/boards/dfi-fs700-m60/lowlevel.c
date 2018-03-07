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
#include <common.h>
#include <linux/sizes.h>
#include <io.h>
#include <asm/sections.h>
#include <asm/mmu.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/imx6-regs.h>
#include <mach/generic.h>

#include <debug_ll.h>

static inline void early_uart_init(void)
{
	writel(0x00000000, MX6_UART1_BASE_ADDR + 0x80);
	writel(0x00004027, MX6_UART1_BASE_ADDR + 0x84);
	writel(0x00000704, MX6_UART1_BASE_ADDR + 0x88);
	writel(0x00000a81, MX6_UART1_BASE_ADDR + 0x90);
	writel(0x0000002b, MX6_UART1_BASE_ADDR + 0x9c);
	writel(0x00013880, MX6_UART1_BASE_ADDR + 0xb0);
	writel(0x0000047f, MX6_UART1_BASE_ADDR + 0xa4);
	writel(0x0000c34f, MX6_UART1_BASE_ADDR + 0xa8);
	writel(0x00000001, MX6_UART1_BASE_ADDR + 0x80);

	putc_ll('>');
}

static inline void early_uart_init_6q(void)
{
	writel(0x3, MX6_IOMUXC_BASE_ADDR + 0x280);
	writel(0x3, MX6_IOMUXC_BASE_ADDR + 0x284);
	writel(0x1, MX6_IOMUXC_BASE_ADDR + 0x920);
	writel(0x0001b0b1, MX6_IOMUXC_BASE_ADDR + 0x650);
	writel(0x0001b0b1, MX6_IOMUXC_BASE_ADDR + 0x654);

	early_uart_init();
}

static inline void early_uart_init_6s(void)
{
	writel(0x3, MX6_IOMUXC_BASE_ADDR + 0x4c);
	writel(0x3, MX6_IOMUXC_BASE_ADDR + 0x50);
	writel(0x1, MX6_IOMUXC_BASE_ADDR + 0x8fc);
	writel(0x0001b0b1, MX6_IOMUXC_BASE_ADDR + 0x360);
	writel(0x0001b0b1, MX6_IOMUXC_BASE_ADDR + 0x364);

	early_uart_init();
}

static inline unsigned int memsize_512M_1G(void)
{
	volatile u32 *a = (u32 *)0x10000000;
	volatile u32 *b = (u32 *)0x30000000;
	u32 size;

	*a = 0x55555555;
	*b = 0xaaaaaaaa;

	if (*a == 0xaaaaaaaa)
		size = SZ_512M;
	else
		size = SZ_1G;

	*a = size;

	return size;
}

static inline unsigned int memsize_1G_2G(void)
{
	volatile u32 *a = (u32 *)0x10000000;
	volatile u32 *b = (u32 *)0x50000000;
	u32 size;

	*a = 0x55555555;
	*b = 0xaaaaaaaa;

	if (*a == 0xaaaaaaaa)
		size = SZ_1G;
	else
		size = SZ_2G;

	*a = size;

	return size;
}

extern char __dtb_imx6q_dfi_fs700_m60_6q_start[];

ENTRY_FUNCTION(start_imx6q_dfi_fs700_m60_6q_nanya, r0, r1, r2)
{
	void *fdt;
	int i;

	imx6_cpu_lowlevel_init();

	arm_setup_stack(0x00940000 - 8);

	for (i = 0x68; i <= 0x80; i += 4)
		writel(0xffffffff, MX6_CCM_BASE_ADDR + i);

	early_uart_init_6q();

	fdt = __dtb_imx6q_dfi_fs700_m60_6q_start + get_runtime_offset();

	barebox_arm_entry(0x10000000, memsize_1G_2G(), fdt);
}

ENTRY_FUNCTION(start_imx6q_dfi_fs700_m60_6q_micron, r0, r1, r2)
{
	void *fdt;
	int i;

	imx6_cpu_lowlevel_init();

	arm_setup_stack(0x00940000 - 8);

	for (i = 0x68; i <= 0x80; i += 4)
		writel(0xffffffff, MX6_CCM_BASE_ADDR + i);

	early_uart_init_6q();

	fdt = __dtb_imx6q_dfi_fs700_m60_6q_start + get_runtime_offset();

	*(uint32_t *)0x10000000 = SZ_1G;

	barebox_arm_entry(0x10000000, SZ_1G, fdt);
}

extern char __dtb_imx6dl_dfi_fs700_m60_6s_start[];

ENTRY_FUNCTION(start_imx6dl_dfi_fs700_m60_6s, r0, r1, r2)
{
	void *fdt;
	int i;

	imx6_cpu_lowlevel_init();

	arm_setup_stack(0x00920000 - 8);

	for (i = 0x68; i <= 0x80; i += 4)
		writel(0xffffffff, MX6_CCM_BASE_ADDR + i);

	early_uart_init_6s();

	fdt = __dtb_imx6dl_dfi_fs700_m60_6s_start + get_runtime_offset();

	barebox_arm_entry(0x10000000, memsize_512M_1G(), fdt);
}
