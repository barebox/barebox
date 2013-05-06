/*
 * Copyright (C) 2013 Lucas Stach <l.stach@pengutronix.de>
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

static struct NS16550_plat debug_uart = {
	.clock = 216000000, /* pll_p rate */
	.shift = 2,
};

static int tegra20_add_debug_console(void)
{
	unsigned long base = 0;

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

	add_ns16550_device(DEVICE_ID_DYNAMIC, base, 8 << debug_uart.shift,
			   IORESOURCE_MEM_8BIT, &debug_uart);

	return 0;
}
console_initcall(tegra20_add_debug_console);

static int tegra20_mem_init(void)
{
	arm_add_mem_device("ram0", 0x0, tegra20_get_ramsize());

	return 0;
}
mem_initcall(tegra20_mem_init);
