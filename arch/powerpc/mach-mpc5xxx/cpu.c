/*
 * (C) Copyright 2000-2003
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

/*
 * CPU specific code for the MPC5xxx CPUs
 */

#include <common.h>
#include <command.h>
#include <mach/mpc5xxx.h>
#include <asm/processor.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include <init.h>
#include <types.h>
#include <errno.h>
#include <of.h>
#include <restart.h>
#include <mach/clock.h>
#include <asm-generic/memory_layout.h>
#include <memory.h>

int checkcpu (void)
{
	ulong clock = get_cpu_clock();
	uint svr, pvr;

	puts ("CPU:   ");

	svr = get_svr();
	pvr = get_pvr();
	switch (SVR_VER (svr)) {
	case SVR_MPC5200:
		printf ("MPC5200");
		break;
	default:
		printf ("MPC52??  (SVR %08x)", svr);
		break;
	}

	printf (" v%d.%d, Core v%d.%d", SVR_MJREV (svr), SVR_MNREV (svr),
		PVR_MAJ(pvr), PVR_MIN(pvr));
	printf (" at %ld Hz\n", clock);
	return 0;
}

/* ------------------------------------------------------------------------- */

static int mpc5xxx_reserve_region(void)
{
	struct resource *r;

	/* keep this in sync with the assembler routines setting up the stack */
	r = request_sdram_region("stack", _text_base - STACK_SIZE, STACK_SIZE);
	if (r == NULL) {
		pr_err("Failed to request stack region at: 0x%08lx/0x%08lx\n",
			_text_base - STACK_SIZE, _text_base - 1);
		return -EBUSY;
	}

	return 0;
}
coredevice_initcall(mpc5xxx_reserve_region);

static void __noreturn mpc5xxx_restart_soc(struct restart_handler *rst)
{
	ulong msr;
	/* Interrupts and MMU off */
	__asm__ __volatile__ ("mfmsr    %0":"=r" (msr):);

	msr &= ~(MSR_ME | MSR_EE | MSR_IR | MSR_DR);
	__asm__ __volatile__ ("mtmsr    %0"::"r" (msr));

	/* Charge the watchdog timer */
	*(vu_long *)(MPC5XXX_GPT0_COUNTER) = 0x0001000f;
	*(vu_long *)(MPC5XXX_GPT0_ENABLE) = 0x9004; /* wden|ce|timer_ms */
	hang();
}

static int restart_register_feature(void)
{
	return restart_handler_register_fn(mpc5xxx_restart_soc);
}
coredevice_initcall(restart_register_feature);

/* ------------------------------------------------------------------------- */

#ifdef CONFIG_OFTREE
static int of_mpc5200_fixup(struct device_node *root, void *unused)
{
	struct device_node *node;

	int div = in_8((void*)CFG_MBAR + 0x204) & 0x0020 ? 8 : 4;

	node = of_find_node_by_path_from(root, "/cpus/PowerPC,5200@0");
	if (!node) {
		pr_err("Cannot find node '/cpus/PowerPC,5200@0' for proper CPU frequency fixup\n");
		return -EINVAL;
	}

	of_property_write_u32(node, "timebase-frequency", get_timebase_clock());
	of_property_write_u32(node, "bus-frequency", get_bus_clock());
	of_property_write_u32(node, "clock-frequency", get_cpu_clock());

	node = of_find_node_by_path_from(root, "/soc5200@f0000000");
	if (!node) {
		pr_err("Cannot find node '/soc5200@f0000000' for proper SOC frequency fixup\n");
		return -EINVAL;
	}

	of_property_write_u32(node, "bus-frequency", get_ipb_clock());
	of_property_write_u32(node, "system-frequency", get_bus_clock() * div);

	return 0;
}

static int of_register_mpc5200_fixup(void)
{
	return of_register_fixup(of_mpc5200_fixup, NULL);
}
late_initcall(of_register_mpc5200_fixup);
#endif

unsigned long mpc5200_get_sdram_size(unsigned int cs)
{
	unsigned long size;

	if (cs > 1)
		return 0;

	/* retrieve size of memory connected to SDRAM CS0 */
	size = *(vu_long *)(MPC5XXX_SDRAM_CS0CFG + (cs * 4)) & 0xFF;
	if (size >= 0x13)
		size = (1 << (size - 0x13)) << 20;
	else
		size = 0;

	return size;
}

int mpc5200_setup_bus_clocks(unsigned int ipbdiv, unsigned long pcidiv)
{
	u32 cdmcfg = *(vu_long *)MPC5XXX_CDM_CFG;

	cdmcfg &= ~0x103;

	switch (ipbdiv) {
	case 1:
		break;
	case 2:
		cdmcfg |= 0x100;
		break;
	default:
		return -EINVAL;
	}

	switch (pcidiv) {
	case 1:
		if (ipbdiv == 2)
			return -EINVAL;
		break;
	case 2:
		if (ipbdiv == 1)
			cdmcfg |= 0x1; /* ipb / 2 */
		break;
	case 4:
		cdmcfg |= 0x2; /* xlb / 4 */
		break;
	default:
		return -EINVAL;
	}

	*(vu_long *)MPC5XXX_CDM_CFG = cdmcfg;

	return 0;
}

struct mpc5200_cs {
	void *start;
	void *stop;
	void *cfg;
	unsigned int addecr;
};

static struct mpc5200_cs chipselects[] = {
	{
		.start = (void *)MPC5XXX_CS0_START,
		.stop = (void *)MPC5XXX_CS0_STOP,
		.cfg = (void *)MPC5XXX_CS0_CFG,
		.addecr = 1 << 16,
	}, {
		.start = (void *)MPC5XXX_CS1_START,
		.stop = (void *)MPC5XXX_CS1_STOP,
		.cfg = (void *)MPC5XXX_CS1_CFG,
		.addecr = 1 << 17,
	}, {
		.start = (void *)MPC5XXX_CS2_START,
		.stop = (void *)MPC5XXX_CS2_STOP,
		.cfg = (void *)MPC5XXX_CS2_CFG,
		.addecr = 1 << 18,
	}, {
		.start = (void *)MPC5XXX_CS3_START,
		.stop = (void *)MPC5XXX_CS3_STOP,
		.cfg = (void *)MPC5XXX_CS3_CFG,
		.addecr = 1 << 19,
	}, {
		.start = (void *)MPC5XXX_CS4_START,
		.stop = (void *)MPC5XXX_CS4_STOP,
		.cfg = (void *)MPC5XXX_CS4_CFG,
		.addecr = 1 << 20,
	}, {
		.start = (void *)MPC5XXX_CS5_START,
		.stop = (void *)MPC5XXX_CS5_STOP,
		.cfg = (void *)MPC5XXX_CS5_CFG,
		.addecr = 1 << 21,
	}, {
		.start = (void *)MPC5XXX_CS6_START,
		.stop = (void *)MPC5XXX_CS6_STOP,
		.cfg = (void *)MPC5XXX_CS6_CFG,
		.addecr = 1 << 26,
	}, {
		.start = (void *)MPC5XXX_CS7_START,
		.stop = (void *)MPC5XXX_CS7_STOP,
		.cfg = (void *)MPC5XXX_CS7_CFG,
		.addecr = 1 << 27,
	}, {
		.start = (void *)MPC5XXX_BOOTCS_START,
		.stop = (void *)MPC5XXX_BOOTCS_STOP,
		.cfg = (void *)MPC5XXX_CS0_CFG,
		.addecr = 1 << 25,
	},
};

void mpc5200_setup_cs(int cs, unsigned long start, unsigned long size, u32 cfg)
{
	u32 addecr;

	out_be32(chipselects[cs].start, START_REG(start));
	out_be32(chipselects[cs].stop, STOP_REG(start, size));
	out_be32(chipselects[cs].cfg, cfg);

	addecr = in_be32((void *)MPC5XXX_ADDECR);
	addecr |= chipselects[cs].addecr | 1;
	out_be32((void *)MPC5XXX_ADDECR, addecr);
}
