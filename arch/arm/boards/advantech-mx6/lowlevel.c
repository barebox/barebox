// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2018 Christoph Fritz <chf.fritz@googlemail.com>

#include <debug_ll.h>
#include <common.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <image-metadata.h>
#include <mach/generic.h>
#include <mach/esdctl.h>
#include <mach/iomux-mx6.h>
#include <linux/sizes.h>

static inline void setup_uart(void)
{
	void __iomem *iomuxbase = (void *)MX6_IOMUXC_BASE_ADDR;

	imx6_ungate_all_peripherals();

	imx_setup_pad(iomuxbase, MX6Q_PAD_CSI0_DAT10__UART1_TXD);

	imx6_uart_setup_ll();

	putc_ll('>');
}

extern char __dtb_imx6dl_advantech_rom_7421_start[];

ENTRY_FUNCTION(start_advantech_imx6dl_rom_7421, r0, r1, r2)
{
	imx6_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();
	barrier();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	imx6q_barebox_entry(__dtb_imx6dl_advantech_rom_7421_start);
}
