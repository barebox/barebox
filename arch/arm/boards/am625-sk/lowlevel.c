// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/k3/debug_ll.h>
#include <debug_ll.h>
#include <pbl.h>
#include <cache.h>
#include <mach/k3/r5.h>
#include <pbl/handoff-data.h>
#include <compressed-dtb.h>
#include <mach/k3/common.h>

#include "ddr.h"

static noinline void am625_sk_continue(void)
{
	unsigned long membase = 0x80000000, memsize;
	extern char __dtb_z_k3_am625_sk_start[];

	memsize = am62x_sdram_size();

	pr_info("Detected DRAM size: %ldMiB\n", memsize >> 20);

	if (memsize > SZ_2G)
		memsize = SZ_2G; /* only need initial memory here */
	
	if (memsize == SZ_512M)
		memsize = SZ_512M - 0x04000000; /* substract space needed for TF-A, OP-TEE, ... */

	barebox_arm_entry(membase, memsize, __dtb_z_k3_am625_sk_start);
}

ENTRY_FUNCTION_WITHSTACK(start_am625_sk, 0x80800000, r0, r1, r2)
{
	putc_ll('>');

	arm_cpu_lowlevel_init();

	relocate_to_current_adr();

	setup_c();

	am625_sk_continue();
}

static noinline void am625_sk_r5_continue(void)
{
	extern char __dtb_z_k3_am625_r5_sk_start[];

	pbl_set_putc((void *)debug_ll_ns16550_putc, IOMEM(AM62X_UART_UART0_BASE));

	putc_ll('>');

	k3_mpu_setup_regions();

	am625_early_init();
	am625_sk_ddr_init();

	barebox_arm_entry(0x80000000, SZ_2G, __dtb_z_k3_am625_r5_sk_start);
}

void am625_sk_r5_entry(void);

void am625_sk_r5_entry(void)
{
	k3_ctrl_mmr_unlock();

	writel(0x00050000, 0xf41c8);
	writel(0x00010000, 0xf41cc);

	k3_debug_ll_init(IOMEM(AM62X_UART_UART0_BASE));

	relocate_to_current_adr();
	setup_c();

	am625_sk_r5_continue();
}

static noinline void am625sip_sk_r5_continue(void)
{
	extern char __dtb_z_k3_am625sip_r5_sk_start[];

	pbl_set_putc((void *)debug_ll_ns16550_putc, IOMEM(AM62X_UART_UART0_BASE));

	putc_ll('>');

	k3_mpu_setup_regions();

	am625_early_init();
	am625sip_sk_ddr_init();

	barebox_arm_entry(0x80000000, SZ_512M - 0x04000000, __dtb_z_k3_am625sip_r5_sk_start);
}

void am625sip_sk_r5_entry(void);

void am625sip_sk_r5_entry(void)
{
	k3_ctrl_mmr_unlock();

	writel(0x00050000, 0xf41c8);
	writel(0x00010000, 0xf41cc);

	k3_debug_ll_init(IOMEM(AM62X_UART_UART0_BASE));

	relocate_to_current_adr();
	setup_c();

	am625sip_sk_r5_continue();
}
