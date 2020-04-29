/*
 * (C) Copyright 2000-2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
#include <mach/mpc5xxx.h>
#include <init.h>
#include <asm/processor.h>
#include <types.h>

/* Bus-to-Core Multipliers */

static int bus2core[] = {
	3, 2, 2, 2, 4, 4, 5, 9,
	6, 11, 8, 10, 3, 12, 7, 0,
	6, 5, 13, 2, 14, 4, 15, 9,
	0, 11, 8, 10, 16, 12, 7, 0
};

unsigned long get_bus_clock(void)
{
	unsigned long val, vco;

#if !defined(CFG_MPC5XXX_CLKIN)
#error clock measuring not implemented yet - define CFG_MPC5XXX_CLKIN
#endif

	val = *(vu_long *)MPC5XXX_CDM_PORCFG;
	if (val & (1 << 6))
		vco = CFG_MPC5XXX_CLKIN * 12;
	else
		vco = CFG_MPC5XXX_CLKIN * 16;

	if (val & (1 << 5))
		return vco / 8;
	else
		return vco / 4;
}

unsigned long get_cpu_clock(void)
{
	unsigned long val;
	val = *(vu_long *)MPC5XXX_CDM_PORCFG;
	return get_bus_clock() * bus2core[val & 0x1f] / 2;
}

unsigned long get_ipb_clock(void)
{
	unsigned long val;

	val = *(vu_long *)MPC5XXX_CDM_CFG;
	if (val & (1 << 8))
		return get_bus_clock() / 2;
	else
		return get_bus_clock();
}

unsigned long get_pci_clock(void)
{
	unsigned long val;

	val = *(vu_long *)MPC5XXX_CDM_CFG;
	switch (val & 3) {
		case 0:
			return get_ipb_clock();
		case 1:
			return get_ipb_clock() / 2;
		default:
			return get_bus_clock() / 4;
	}
}

unsigned long get_timebase_clock(void)
{
	return (get_bus_clock() + 3L) / 4L;
}

int prt_mpc5xxx_clks (void)
{
	printf("       Bus %ld MHz, IPB %ld MHz, PCI %ld MHz\n",
			get_bus_clock() / 1000000, get_ipb_clock() / 1000000,
			get_pci_clock() / 1000000);

	return 0;
}

late_initcall(prt_mpc5xxx_clks);

