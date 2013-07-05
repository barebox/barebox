/*
 * Copyright
 * (C) 2013 Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
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
#include <ns16550.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <asm/memory.h>
#include <mach/dove-regs.h>

#define CONSOLE_UART_BASE	DOVE_UARTn_BASE(CONFIG_MVEBU_CONSOLE_UART)

static struct clk *tclk;

static inline void dove_remap_mc_regs(void)
{
	void __iomem *mcboot = IOMEM(DOVE_BOOTUP_MC_REGS);
	uint32_t val;

	/* remap ahb slave base */
	val  = readl(DOVE_CPU_CTRL) & 0xffff0000;
	val |= (DOVE_REMAP_MC_REGS & 0xffff0000) >> 16;
	writel(val, DOVE_CPU_CTRL);

	/* remap axi bridge address */
	val  = readl(DOVE_AXI_CTRL) & 0x007fffff;
	val |= DOVE_REMAP_MC_REGS & 0xff800000;
	writel(val, DOVE_AXI_CTRL);

	/* remap memory controller base address */
	val = readl(mcboot + SDRAM_REGS_BASE_DECODE) & 0x0000ffff;
	val |= DOVE_REMAP_MC_REGS & 0xffff0000;
	writel(val, mcboot + SDRAM_REGS_BASE_DECODE);
}

static inline void dove_memory_find(unsigned long *phys_base,
				    unsigned long *phys_size)
{
	int n;

	*phys_base = ~0;
	*phys_size = 0;

	for (n = 0; n < 2; n++) {
		uint32_t map = readl(DOVE_SDRAM_BASE + SDRAM_MAPn(n));
		uint32_t base, size;

		/* skip disabled areas */
		if ((map & SDRAM_MAP_VALID) != SDRAM_MAP_VALID)
			continue;

		base = map & SDRAM_START_MASK;
		if (base < *phys_base)
			*phys_base = base;

		/* real size is encoded as ld(2^(16+length)) */
		size = (map & SDRAM_LENGTH_MASK) >> SDRAM_LENGTH_SHIFT;
		*phys_size += 1 << (16 + size);
	}
}

static struct NS16550_plat uart_plat = {
	.shift = 2,
};

static int dove_add_uart(void)
{
	uart_plat.clock = clk_get_rate(tclk);
	if (!add_ns16550_device(DEVICE_ID_DYNAMIC,
				(unsigned int)CONSOLE_UART_BASE, 32,
				IORESOURCE_MEM_32BIT, &uart_plat))
		return -ENODEV;
	return 0;
}

/*
 * Dove TCLK sample-at-reset configuation
 *
 * SAR0[24:23] : TCLK frequency
 *		 0 = 166 MHz
 *		 1 = 125 MHz
 *		 others reserved.
 */
static int dove_init_clocks(void)
{
	uint32_t strap, sar = readl(DOVE_SAR_BASE + SAR0);
	unsigned int rate;

	strap = (sar & TCLK_FREQ_MASK) >> TCLK_FREQ_SHIFT;
	switch (strap) {
	case 0:
		rate = 166666667;
		break;
	case 1:
		rate = 125000000;
		break;
	default:
		panic("Unknown TCLK strapping %d\n", strap);
	}

	tclk = clk_fixed("tclk", rate);
	return 0;
}

static int dove_init_soc(void)
{
	unsigned long phys_base, phys_size;

	dove_remap_mc_regs();
	dove_init_clocks();
	clkdev_add_physbase(tclk, (unsigned int)DOVE_TIMER_BASE, NULL);
	clkdev_add_physbase(tclk, (unsigned int)DOVE_SPI0_BASE, NULL);
	clkdev_add_physbase(tclk, (unsigned int)DOVE_SPI1_BASE, NULL);
	add_generic_device("orion-timer", DEVICE_ID_SINGLE, NULL,
			   (unsigned int)DOVE_TIMER_BASE, 0x30,
			   IORESOURCE_MEM, NULL);
	dove_memory_find(&phys_base, &phys_size);
	arm_add_mem_device("ram0", phys_base, phys_size);
	dove_add_uart();

	return 0;
}
core_initcall(dove_init_soc);

void __noreturn reset_cpu(unsigned long addr)
{
	/* enable and assert RSTOUTn */
	writel(SOFT_RESET_OUT_EN, DOVE_BRIDGE_BASE + BRIDGE_RSTOUT_MASK);
	writel(SOFT_RESET_EN, DOVE_BRIDGE_BASE + BRIDGE_SYS_SOFT_RESET);
	while (1)
		;
}
EXPORT_SYMBOL(reset_cpu);
