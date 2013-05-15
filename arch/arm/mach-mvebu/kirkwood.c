/*
 * Copyright (C) 2013 Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
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
#include <init.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <ns16550.h>
#include <mach/kirkwood-regs.h>
#include <asm/memory.h>
#include <asm/barebox-arm.h>

static struct clk *tclk;

static inline void kirkwood_memory_find(unsigned long *phys_base,
				     unsigned long *phys_size)
{
	void __iomem *sdram_win = IOMEM(KIRKWOOD_SDRAM_WIN_BASE);
	int cs;

	*phys_base = ~0;
	*phys_size = 0;

	for (cs = 0; cs < 4; cs++) {
		uint32_t base = readl(sdram_win + DDR_BASE_CS_OFF(cs));
		uint32_t ctrl = readl(sdram_win + DDR_SIZE_CS_OFF(cs));

		/* Skip non-enabled CS */
		if (! (ctrl & DDR_SIZE_ENABLED))
			continue;

		base &= DDR_BASE_CS_LOW_MASK;
		if (base < *phys_base)
			*phys_base = base;
		*phys_size += (ctrl | ~DDR_SIZE_MASK) + 1;
	}
}

void __naked __noreturn kirkwood_barebox_entry(void)
{
	unsigned long phys_base, phys_size;
	kirkwood_memory_find(&phys_base, &phys_size);
	writel('E', 0xD0012000);
	barebox_arm_entry(phys_base, phys_size, 0);
}

static struct NS16550_plat uart_plat = {
	.shift = 2,
};

int kirkwood_add_uart0(void)
{
	uart_plat.clock = clk_get_rate(tclk);
	if (!add_ns16550_device(DEVICE_ID_DYNAMIC,
				(unsigned int)KIRKWOOD_UART_BASE,
				32, IORESOURCE_MEM_32BIT, &uart_plat))
		return -ENODEV;
	return 0;
}

static int kirkwood_init_clocks(void)
{
	uint32_t sar = readl(KIRKWOOD_SAR_BASE);
	unsigned int rate;

	if (sar & (1 << KIRKWOOD_TCLK_BIT))
		rate = 166666667;
	else
		rate = 200000000;

	tclk = clk_fixed("tclk", rate);
	return clk_register_clkdev(tclk, NULL, "orion-timer");
}

static int kirkwood_init_soc(void)
{
	unsigned long phys_base, phys_size;

	kirkwood_init_clocks();
	add_generic_device("orion-timer", DEVICE_ID_SINGLE, NULL,
			   (unsigned int)KIRKWOOD_TIMER_BASE, 0x30,
			   IORESOURCE_MEM, NULL);
	kirkwood_memory_find(&phys_base, &phys_size);
	arm_add_mem_device("ram0", phys_base, phys_size);

	return 0;
}

postcore_initcall(kirkwood_init_soc);

void __noreturn reset_cpu(unsigned long addr)
{
	writel(0x4, KIRKWOOD_CPUCTRL_BASE + 0x8);
	writel(0x1, KIRKWOOD_CPUCTRL_BASE + 0xC);
	for(;;)
		;
}
EXPORT_SYMBOL(reset_cpu);
