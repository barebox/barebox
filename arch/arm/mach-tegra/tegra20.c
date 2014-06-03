/*
 * Copyright (C) 2013-2014 Lucas Stach <l.stach@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <init.h>
#include <ns16550.h>
#include <asm/memory.h>
#include <mach/iomap.h>
#include <mach/lowlevel.h>
#include <mach/tegra114-sysctr.h>

static struct NS16550_plat debug_uart = {
	.shift = 2,
};

static int tegra_add_debug_console(void)
{
	unsigned long base = 0;

	if (!of_machine_is_compatible("nvidia,tegra20") &&
	    !of_machine_is_compatible("nvidia,tegra30") &&
	    !of_machine_is_compatible("nvidia,tegra124"))
		return 0;

	/* figure out which UART to use */
	if (IS_ENABLED(CONFIG_TEGRA_UART_NONE))
		return 0;
	if (IS_ENABLED(CONFIG_TEGRA_UART_ODMDATA))
		base = tegra20_get_debuguart_base();
	if (IS_ENABLED(CONFIG_TEGRA_UART_A))
		base = TEGRA_UARTA_BASE;
	if (IS_ENABLED(CONFIG_TEGRA_UART_B))
		base = TEGRA_UARTB_BASE;
	if (IS_ENABLED(CONFIG_TEGRA_UART_C))
		base = TEGRA_UARTC_BASE;
	if (IS_ENABLED(CONFIG_TEGRA_UART_D))
		base = TEGRA_UARTD_BASE;
	if (IS_ENABLED(CONFIG_TEGRA_UART_E))
		base = TEGRA_UARTE_BASE;

	if (!base)
		return -ENODEV;

	debug_uart.clock = tegra_get_pllp_rate();

	add_ns16550_device(DEVICE_ID_DYNAMIC, base, 8 << debug_uart.shift,
			   IORESOURCE_MEM | IORESOURCE_MEM_8BIT, &debug_uart);

	return 0;
}
console_initcall(tegra_add_debug_console);

static int tegra20_mem_init(void)
{
	if (!of_machine_is_compatible("nvidia,tegra20"))
		return 0;

	arm_add_mem_device("ram0", 0x0, tegra20_get_ramsize());

	return 0;
}
mem_initcall(tegra20_mem_init);

static int tegra30_mem_init(void)
{
	if (!of_machine_is_compatible("nvidia,tegra30") &&
	    !of_machine_is_compatible("nvidia,tegra124"))
		return 0;

	arm_add_mem_device("ram0", SZ_2G, tegra30_get_ramsize());

	return 0;
}
mem_initcall(tegra30_mem_init);

static int tegra114_architected_timer_init(void)
{
	u32 freq, reg;

	if (!of_machine_is_compatible("nvidia,tegra114") &&
	    !of_machine_is_compatible("nvidia,tegra124"))
		return 0;

	freq = tegra_get_osc_clock();

	/* ARM CNTFRQ */
	asm("mcr p15, 0, %0, c14, c0, 0\n" : : "r" (freq));

	/* Tegra specific SYSCTR */
	writel(freq, TEGRA_SYSCTR0_BASE + TEGRA_SYSCTR0_CNTFID0);

	reg = readl(TEGRA_SYSCTR0_BASE + TEGRA_SYSCTR0_CNTCR);
	reg |= TEGRA_SYSCTR0_CNTCR_ENABLE | TEGRA_SYSCTR0_CNTCR_HDBG;
	writel(reg, TEGRA_SYSCTR0_BASE + TEGRA_SYSCTR0_CNTCR);

	return 0;
}
coredevice_initcall(tegra114_architected_timer_init);
