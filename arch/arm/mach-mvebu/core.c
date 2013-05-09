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
#include <asm/memory.h>
#include <asm/barebox-arm.h>

#define MVEBU_INT_REGS_BASE (0xd0000000)
#define  MVEBU_UART0_BASE     (MVEBU_INT_REGS_BASE + 0x12000)
#define  MVEBU_SYSCTL_BASE    (MVEBU_INT_REGS_BASE + 0x18200)
#define  MVEBU_SDRAM_WIN_BASE (MVEBU_INT_REGS_BASE + 0x20180)
#define  MVEBU_TIMER_BASE     (MVEBU_INT_REGS_BASE + 0x20300)
#define  MVEBU_SAR_BASE       (MVEBU_INT_REGS_BASE + 0x18230)

#define DDR_BASE_CS_OFF(n)     (0x0000 + ((n) << 3))
#define  DDR_BASE_CS_HIGH_MASK  0xf
#define  DDR_BASE_CS_LOW_MASK   0xff000000
#define DDR_SIZE_CS_OFF(n)     (0x0004 + ((n) << 3))
#define  DDR_SIZE_ENABLED       (1 << 0)
#define  DDR_SIZE_CS_MASK       0x1c
#define  DDR_SIZE_CS_SHIFT      2
#define  DDR_SIZE_MASK          0xff000000

#define SAR_LOW_REG_OFF         0
#define  SAR_TCLK_FREQ_BIT      20
#define SAR_HIGH_REG_OFF        0x4

static struct clk *tclk;

static inline void mvebu_memory_find(unsigned long *phys_base,
				     unsigned long *phys_size)
{
	void __iomem *sdram_win = IOMEM(MVEBU_SDRAM_WIN_BASE);
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

void __naked __noreturn mvebu_barebox_entry(void)
{
	unsigned long phys_base, phys_size;
	mvebu_memory_find(&phys_base, &phys_size);
	barebox_arm_entry(phys_base, phys_size, 0);
}

static struct NS16550_plat uart0_plat = {
	.shift = 2,
};

int mvebu_add_uart0(void)
{
	uart0_plat.clock = clk_get_rate(tclk);
	add_ns16550_device(DEVICE_ID_DYNAMIC, MVEBU_UART0_BASE, 32,
			   IORESOURCE_MEM_32BIT, &uart0_plat);
	return 0;
}

#if defined(CONFIG_ARCH_ARMADA_370)
static int mvebu_init_clocks(void)
{
	uint32_t val;
	unsigned int rate;
	void __iomem *sar = IOMEM(MVEBU_SAR_BASE) + SAR_LOW_REG_OFF;

	val = readl(sar);

	/* On Armada 370, the TCLK frequency can be either 166 Mhz or
	 * 200 Mhz */
	if (val & (1 << SAR_TCLK_FREQ_BIT))
		rate = 200 * 1000 * 1000;
	else
		rate = 166 * 1000 * 1000;

	tclk = clk_fixed("tclk", rate);
	return clk_register_clkdev(tclk, NULL, "mvebu-timer");
}
#endif

#if defined(CONFIG_ARCH_ARMADA_XP)
static int mvebu_init_clocks(void)
{
	/* On Armada XP, the TCLK frequency is always 250 Mhz */
	tclk = clk_fixed("tclk", 250 * 1000 * 1000);
	return clk_register_clkdev(tclk, NULL, "mvebu-timer");
}
#endif

static int mvebu_init_soc(void)
{
	unsigned long phys_base, phys_size;

	mvebu_init_clocks();
	add_generic_device("mvebu-timer", DEVICE_ID_SINGLE, NULL,
			   MVEBU_TIMER_BASE, 0x30, IORESOURCE_MEM,
			   NULL);
	mvebu_memory_find(&phys_base, &phys_size);
	arm_add_mem_device("ram0", phys_base, phys_size);
	return 0;
}

postcore_initcall(mvebu_init_soc);

void __noreturn reset_cpu(unsigned long addr)
{
	writel(0x1, MVEBU_SYSCTL_BASE + 0x60);
	writel(0x1, MVEBU_SYSCTL_BASE + 0x64);
	while (1)
		;
}
EXPORT_SYMBOL(reset_cpu);
