// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/k3/debug_ll.h>
#include <debug_ll.h>
#include <pbl.h>
#include <cache.h>
#include <pbl/handoff-data.h>
#include <compressed-dtb.h>
#include <mach/k3/common.h>

#define SECURE_SIZE 0x600000

static noinline void am62lx_evm_continue(void)
{
	extern char __dtb_z_k3_am62l3_evm_start[];

	pbl_set_putc((void *)debug_ll_ns16550_putc, IOMEM(AM62X_UART_UART0_BASE));

	putc_ll('>');

	barebox_arm_entry(0x80000000 + SECURE_SIZE, SZ_2G - SECURE_SIZE, __dtb_z_k3_am62l3_evm_start);
}

ENTRY_FUNCTION_WITHSTACK(start_am62lx_evm, 0x80800000, r0, r1, r2)
{
	writel(0x00000000, 0x040841b8);

	k3_debug_ll_init(IOMEM(AM62X_UART_UART0_BASE));

	relocate_to_current_adr();
	setup_c();

	am62lx_evm_continue();
}
