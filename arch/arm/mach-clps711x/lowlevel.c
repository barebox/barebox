// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: Alexander Shiyan <shc_work@mail.ru>

#include <asm/io.h>
#include <asm/barebox-arm.h>
#include <common.h>
#include <debug_ll.h>
#include <linux/sizes.h>
#include <mach/clps711x.h>

#define DEBUG_LL_BAUDRATE	(57600)

static inline void setup_uart(const u32 bus_speed)
{
	u32 baud_base = DIV_ROUND_CLOSEST(bus_speed, 10);
	u32 baud_divisor =
		DIV_ROUND_CLOSEST(baud_base, DEBUG_LL_BAUDRATE * 16) - 1;

	writel(baud_divisor | UBRLCR_FIFOEN | UBRLCR_WRDLEN8, UBRLCR1);
	writel(0, STFCLR);
	writel(SYSCON_UARTEN, SYSCON1);

	putc_ll('>');
}

void clps711x_start(void *fdt)
{
	u32 bus, pll;

	/* Check if we running from external 13 MHz clock */
	if (!(readl(SYSFLG2) & SYSFLG2_CKMODE)) {
		/* Setup bus wait state scaling factor to 2  */
		writel(SYSCON3_CLKCTL0 | SYSCON3_CLKCTL1, SYSCON3);
		asm("nop");

		if (IS_ENABLED(CONFIG_CLPS711X_RAISE_CPUFREQ)) {
			/* Setup PLL to 92160000 Hz */
			writel(50 << 24, PLLW);
			asm("nop");
		}

		pll = readl(PLLR) >> 24;
		if (pll)
			bus = (pll * 3686400) / 4;
		else
			bus = 73728000 / 4;
	} else {
		bus = 13000000;
		/* Setup bus wait state scaling factor to 1  */
		writel(0, SYSCON3);
		asm("nop");
	}


	/* Disable UART, IrDa, LCD */
	writel(0, SYSCON1);

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart(bus);

	/* CLKEN select, SDRAM width=32 */
	writel(SYSCON2_CLKENSL, SYSCON2);

	/* Setup SDRAM params (64MB, 16Bit*2, CAS=3) */
	writel(SDCONF_CASLAT_3 | SDCONF_SIZE_256 | SDCONF_WIDTH_16 |
	       SDCONF_CLKCTL | SDCONF_ACTIVE, SDCONF);

	/* Setup Refresh Rate (64ms 8K Blocks) */
	writel((64 * bus) / (8192 * 1000), SDRFPR);

	/* Disable PWM */
	writew(0, PMPCON);
	/* Disable LED flasher */
	writew(0, LEDFLSH);

	barebox_arm_entry(SDRAM0_BASE, SZ_8M, fdt);
}
