// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Based on Linux driver:
 *  Copyright (C) 2003 Atheros Communications, Inc.,  All Rights Reserved.
 *  Copyright (C) 2006 FON Technology, SL.
 *  Copyright (C) 2006 Imre Kaloz <kaloz@openwrt.org>
 *  Copyright (C) 2006-2009 Felix Fietkau <nbd@openwrt.org>
 * Ported to Barebox:
 *  Copyright (C) 2013 Oleksij Rempel <linux@rempel-privat.de>
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <platform_data/serial-ns16550.h>
#include <mach/ar231x_platform.h>
#include <mach/ar2312_regs.h>

struct ar231x_board_data ar231x_board;

/*
 * This table is indexed by bits 5..4 of the CLOCKCTL1 register
 * to determine the predevisor value.
 */
static int CLOCKCTL1_PREDIVIDE_TABLE[4] = { 1, 2, 4, 5 };

static unsigned int
ar2312_cpu_frequency(void)
{
	unsigned int predivide_mask, predivide_shift;
	unsigned int multiplier_mask, multiplier_shift;
	unsigned int clock_ctl1, pre_divide_select, pre_divisor, multiplier;
	unsigned int doubler_mask;
	u32 devid;

	devid = __raw_readl((char *)KSEG1ADDR(AR2312_REV));
	devid &= AR2312_REV_MAJ;
	devid >>= AR2312_REV_MAJ_S;
	if (devid == AR2312_REV_MAJ_AR2313) {
		predivide_mask = AR2313_CLOCKCTL1_PREDIVIDE_MASK;
		predivide_shift = AR2313_CLOCKCTL1_PREDIVIDE_SHIFT;
		multiplier_mask = AR2313_CLOCKCTL1_MULTIPLIER_MASK;
		multiplier_shift = AR2313_CLOCKCTL1_MULTIPLIER_SHIFT;
		doubler_mask = AR2313_CLOCKCTL1_DOUBLER_MASK;
	} else { /* AR5312 and AR2312 */
		predivide_mask = AR2312_CLOCKCTL1_PREDIVIDE_MASK;
		predivide_shift = AR2312_CLOCKCTL1_PREDIVIDE_SHIFT;
		multiplier_mask = AR2312_CLOCKCTL1_MULTIPLIER_MASK;
		multiplier_shift = AR2312_CLOCKCTL1_MULTIPLIER_SHIFT;
		doubler_mask = AR2312_CLOCKCTL1_DOUBLER_MASK;
	}

	/*
	 * Clocking is derived from a fixed 40MHz input clock.
	 *
	 *  cpuFreq = InputClock * MULT (where MULT is PLL multiplier)
	 *  sysFreq = cpuFreq / 4	   (used for APB clock, serial,
	 *				    flash, Timer, Watchdog Timer)
	 *
	 *  cntFreq = cpuFreq / 2	   (use for CPU count/compare)
	 *
	 * So, for example, with a PLL multiplier of 5, we have
	 *
	 *  cpuFreq = 200MHz
	 *  sysFreq = 50MHz
	 *  cntFreq = 100MHz
	 *
	 * We compute the CPU frequency, based on PLL settings.
	 */

	clock_ctl1 = __raw_readl((char *)KSEG1ADDR(AR2312_CLOCKCTL1));
	pre_divide_select = (clock_ctl1 & predivide_mask) >> predivide_shift;
	pre_divisor = CLOCKCTL1_PREDIVIDE_TABLE[pre_divide_select];
	multiplier = (clock_ctl1 & multiplier_mask) >> multiplier_shift;

	if (clock_ctl1 & doubler_mask)
		multiplier = multiplier << 1;

	return (40000000 / pre_divisor) * multiplier;
}

static unsigned int
ar2312_sys_frequency(void)
{
	return ar2312_cpu_frequency() / 4;
}

/*
 * shutdown watchdog
 */
static int watchdog_init(void)
{
	pr_debug("Disable watchdog.\n");
	__raw_writeb(AR2312_WD_CTRL_IGNORE_EXPIRATION,
					(char *)KSEG1ADDR(AR2312_WD_CTRL));
	return 0;
}

static void flash_init(void)
{
	u32 ctl, old_ctl;

	/* Configure flash bank 0.
	 * Assume 8M maximum window size on this SoC.
	 * Flash will be aliased if it's smaller
	 */
	old_ctl = __raw_readl((char *)KSEG1ADDR(AR2312_FLASHCTL0));
	ctl = FLASHCTL_E | FLASHCTL_AC_8M | FLASHCTL_RBLE |
			(0x01 << FLASHCTL_IDCY_S) |
			(0x07 << FLASHCTL_WST1_S) |
			(0x07 << FLASHCTL_WST2_S) | (old_ctl & FLASHCTL_MW);

	__raw_writel(ctl, (char *)KSEG1ADDR(AR2312_FLASHCTL0));
	/* Disable other flash banks */
	old_ctl = __raw_readl((char *)KSEG1ADDR(AR2312_FLASHCTL1));
	__raw_writel(old_ctl & ~(FLASHCTL_E | FLASHCTL_AC),
			(char *)KSEG1ADDR(AR2312_FLASHCTL1));

	old_ctl = __raw_readl((char *)KSEG1ADDR(AR2312_FLASHCTL2));
	__raw_writel(old_ctl & ~(FLASHCTL_E | FLASHCTL_AC),
			(char *)KSEG1ADDR(AR2312_FLASHCTL2));

	/* We need to find atheros config. MAC address is there. */
	ar231x_find_config((char *)KSEG1ADDR(AR2312_FLASH +
					     AR2312_MAX_FLASH_SIZE));
}

static int ether_init(void)
{
	static struct resource res[2];
	struct ar231x_eth_platform_data *eth = &ar231x_board.eth_pdata;

	/* Base ETH registers  */
	res[0].start = KSEG1ADDR(AR2312_ENET1);
	res[0].end = res[0].start + 0x100000 - 1;
	res[0].flags = IORESOURCE_MEM;
	/* Base PHY registers */
	res[1].start = KSEG1ADDR(AR2312_ENET0);
	res[1].end = res[1].start + 0x100000 - 1;
	res[1].flags = IORESOURCE_MEM;

	/* MAC address located in atheros config on flash. */
	eth->mac = ar231x_board.config->enet0_mac;

	eth->reset_mac = AR2312_RESET_ENET0 | AR2312_RESET_ENET1;
	eth->reset_phy = AR2312_RESET_EPHY0 | AR2312_RESET_EPHY1;

	eth->reset_bit = ar231x_reset_bit;

	/* FIXME: base_reset should be replaced with reset driver */
	eth->base_reset = KSEG1ADDR(AR2312_RESET);

	add_generic_device_res("ar231x_eth", DEVICE_ID_DYNAMIC, res, 2, eth);
	return 0;
}

static int platform_init(void)
{
	add_generic_device("ar231x_reset", DEVICE_ID_SINGLE, NULL,
			KSEG1ADDR(AR2312_RESET), 0x4,
			IORESOURCE_MEM, NULL);
	watchdog_init();
	flash_init();
	ether_init();
	return 0;
}
late_initcall(platform_init);

static struct NS16550_plat serial_plat = {
	.shift = AR2312_UART_SHIFT,
};

static int ar2312_console_init(void)
{
	u32 reset;

	/* reset UART0 */
	reset = __raw_readl((char *)KSEG1ADDR(AR2312_RESET));
	reset = ((reset & ~AR2312_RESET_APB) | AR2312_RESET_UART0);
	__raw_writel(reset, (char *)KSEG1ADDR(AR2312_RESET));

	reset &= ~AR2312_RESET_UART0;
	__raw_writel(reset, (char *)KSEG1ADDR(AR2312_RESET));

	/* Register the serial port */
	serial_plat.clock = ar2312_sys_frequency();
	add_ns16550_device(DEVICE_ID_DYNAMIC, KSEG1ADDR(AR2312_UART0),
			   8 << AR2312_UART_SHIFT,
			   IORESOURCE_MEM | IORESOURCE_MEM_8BIT,
			   &serial_plat);
	return 0;
}
console_initcall(ar2312_console_init);
