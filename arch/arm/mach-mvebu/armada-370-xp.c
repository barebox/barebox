/*
 * Copyright
 * (C) 2013 Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
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
#include <mach/armada-370-xp-regs.h>

#define CONSOLE_UART_BASE	\
	ARMADA_370_XP_UARTn_BASE(CONFIG_MVEBU_CONSOLE_UART)

static struct clk *tclk;

static inline void armada_370_xp_memory_find(unsigned long *phys_base,
					     unsigned long *phys_size)
{
	int cs;

	*phys_base = ~0;
	*phys_size = 0;

	for (cs = 0; cs < 4; cs++) {
		u32 base = readl(ARMADA_370_XP_SDRAM_BASE + DDR_BASE_CSn(cs));
		u32 ctrl = readl(ARMADA_370_XP_SDRAM_BASE + DDR_SIZE_CSn(cs));

		/* Skip non-enabled CS */
		if ((ctrl & DDR_SIZE_ENABLED) != DDR_SIZE_ENABLED)
			continue;

		base &= DDR_BASE_CS_LOW_MASK;
		if (base < *phys_base)
			*phys_base = base;
		*phys_size += (ctrl | ~DDR_SIZE_MASK) + 1;
	}
}

static struct NS16550_plat uart_plat = {
	.shift = 2,
};

static int armada_370_xp_add_uart(void)
{
	uart_plat.clock = clk_get_rate(tclk);
	if (!add_ns16550_device(DEVICE_ID_DYNAMIC,
				(unsigned int)CONSOLE_UART_BASE, 32,
				IORESOURCE_MEM_32BIT, &uart_plat))
	    return -ENODEV;
	return 0;
}

#if defined(CONFIG_ARCH_ARMADA_370)
static int armada_370_init_clocks(void)
{
	u32 val = readl(ARMADA_370_XP_SAR_BASE + SAR_LOW);
	unsigned int rate;

	/*
	 * On Armada 370, the TCLK frequency can be either
	 * 166 Mhz or 200 Mhz
	 */
	if ((val & SAR_TCLK_FREQ) == SAR_TCLK_FREQ)
		rate = 200000000;
	else
		rate = 166000000;

	tclk = clk_fixed("tclk", rate);
	return clk_register_clkdev(tclk, NULL, "mvebu-timer");
}
#define armada_370_xp_init_clocks()	armada_370_init_clocks()
#endif

#if defined(CONFIG_ARCH_ARMADA_XP)
static int armada_xp_init_clocks(void)
{
	/* On Armada XP, the TCLK frequency is always 250 Mhz */
	tclk = clk_fixed("tclk", 250000000);
	return 0;
}
#define armada_370_xp_init_clocks()	armada_xp_init_clocks()
#endif

static int armada_370_xp_init_soc(void)
{
	unsigned long phys_base, phys_size;

	barebox_set_model("Marvell Armada 370/XP");
	barebox_set_hostname("armada");

	armada_370_xp_init_clocks();
	clkdev_add_physbase(tclk, (unsigned int)ARMADA_370_XP_TIMER_BASE, NULL);
	add_generic_device("mvebu-timer", DEVICE_ID_SINGLE, NULL,
			   (unsigned int)ARMADA_370_XP_TIMER_BASE, 0x30,
			   IORESOURCE_MEM, NULL);
	armada_370_xp_memory_find(&phys_base, &phys_size);
	arm_add_mem_device("ram0", phys_base, phys_size);
	armada_370_xp_add_uart();
	return 0;
}
core_initcall(armada_370_xp_init_soc);

void __noreturn reset_cpu(unsigned long addr)
{
	writel(0x1, ARMADA_370_XP_SYSCTL_BASE + 0x60);
	writel(0x1, ARMADA_370_XP_SYSCTL_BASE + 0x64);
	while (1)
		;
}
EXPORT_SYMBOL(reset_cpu);
