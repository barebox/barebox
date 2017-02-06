/*
 * (C) Copyright 2013
 * Andre Przywara, Linaro <andre.przywara@linaro.org>
 *
 * Routines to transition ARMv7 processors from secure into non-secure state
 * and from non-secure SVC into HYP mode
 * needed to enable ARMv7 virtualization for current hypervisors
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#define pr_fmt(fmt) "secure: " fmt

#include <common.h>
#include <io.h>
#include <asm/gic.h>
#include <asm/system.h>
#include <init.h>
#include <globalvar.h>
#include <asm/arm-smccc.h>
#include <asm-generic/sections.h>
#include <asm/secure.h>

#include "mmu.h"

/* valid bits in CBAR register / PERIPHBASE value */
#define CBAR_MASK			0xFFFF8000

static unsigned int read_id_pfr1(void)
{
	unsigned int reg;

	asm("mrc p15, 0, %0, c0, c1, 1\n" : "=r"(reg));
	return reg;
}

static u32 read_nsacr(void)
{
	unsigned int reg;

	asm("mrc p15, 0, %0, c1, c1, 2\n" : "=r"(reg));
	return reg;
}

static void write_nsacr(u32 val)
{
	asm("mcr p15, 0, %0, c1, c1, 2" : : "r"(val));
}

static void write_mvbar(u32 val)
{
	asm("mcr p15, 0, %0, c12, c0, 1" : : "r"(val));
}

static unsigned long get_cbar(void)
{
	unsigned periphbase;

	/* get the GIC base address from the CBAR register */
	asm("mrc p15, 4, %0, c15, c0, 0\n" : "=r" (periphbase));

	/* the PERIPHBASE can be mapped above 4 GB (lower 8 bits used to
	 * encode this). Bail out here since we cannot access this without
	 * enabling paging.
	 */
	if ((periphbase & 0xff) != 0) {
		pr_err("PERIPHBASE is above 4 GB, no access.\n");
		return -1;
	}

	return periphbase & CBAR_MASK;
}

static unsigned long get_gicd_base_address(void)
{
	return get_cbar() + GIC_DIST_OFFSET;
}

static int cpu_is_virt_capable(void)
{
	return read_id_pfr1() & (1 << 12);
}

static unsigned long get_gicc_base_address(void)
{
	unsigned long adr = get_cbar();

	if (cpu_is_virt_capable())
		adr += GIC_CPU_OFFSET_A15;
	else
		adr += GIC_CPU_OFFSET_A9;

	return adr;
}

#define GICD_IGROUPRn             0x0080

int armv7_init_nonsec(void)
{
	void __iomem *gicd = IOMEM(get_gicd_base_address());
	unsigned itlinesnr, i;
	u32 val;

	/*
	 * the SCR register will be set directly in the monitor mode handler,
	 * according to the spec one should not tinker with it in secure state
	 * in SVC mode. Do not try to read it once in non-secure state,
	 * any access to it will trap.
	 */

	/* enable the GIC distributor */
	val = readl(gicd + GICD_CTLR);
	val |= 0x3;
	writel(val, gicd + GICD_CTLR);

	/* TYPER[4:0] contains an encoded number of available interrupts */
	itlinesnr = readl(gicd + GICD_TYPER) & 0x1f;

	/*
	 * Set all bits in the GIC group registers to one to allow access
	 * from non-secure state. The first 32 interrupts are private per
	 * CPU and will be set later when enabling the GIC for each core
	 */
	for (i = 1; i <= itlinesnr; i++)
		writel(0xffffffff, gicd + GICD_IGROUPRn + 4 * i);

	return 0;
}

/*
 * armv7_secure_monitor_install - install secure monitor
 *
 * This function is entered in secure mode. It installs the secure
 * monitor code and enters it using a smc call. This function is executed
 * on every CPU. We leave this function returns in nonsecure mode.
 */
int __armv7_secure_monitor_install(void)
{
	struct arm_smccc_res res;
	void __iomem *gicd = IOMEM(get_gicd_base_address());
	void __iomem *gicc = IOMEM(get_gicc_base_address());
	u32 nsacr;

	writel(0xffffffff, gicd + GICD_IGROUPRn);

	writel(0x3, gicc + GICC_CTLR);
	writel(0xff, gicc + GICC_PMR);

	nsacr = read_nsacr();
	nsacr |= 0x00043fff; /* all copros allowed in non-secure mode */
	write_nsacr(nsacr);

	write_mvbar((unsigned long)secure_monitor_init_vectors);

	isb();

	/* Initialize the secure monitor */
	arm_smccc_smc(0, 0, 0, 0, 0, 0, 0, 0, &res);

	/* We're in nonsecure mode now */

	return 0;
}

static bool armv7_have_security_extensions(void)
{
	return (read_id_pfr1() & 0xf0) != 0;
}

/*
 * armv7_secure_monitor_install - install secure monitor
 *
 * This function is entered in secure mode. It installs the secure
 * monitor code and enters it using a smc call. This function is executed
 * once on the primary CPU only. We leave this function returns in nonsecure
 * mode.
 */
int armv7_secure_monitor_install(void)
{
	int mmuon;
	unsigned long ttb, vbar;

	if (!armv7_have_security_extensions()) {
		pr_err("Security extensions not implemented.\n");
		return -EINVAL;
	}

	mmuon = get_cr() & CR_M;

	vbar = get_vbar();

	asm volatile ("mrc p15, 0, %0, c2, c0, 0" : "=r"(ttb));

	armv7_init_nonsec();
	__armv7_secure_monitor_install();

	asm volatile ("mcr p15, 0, %0, c2, c0, 0" : : "r"(ttb));

	set_vbar(vbar);

	if (mmuon) {
		/*
		 * If the MMU was already turned on in secure mode, enable it in
		 * non-secure mode aswell
		 */
		__mmu_cache_on();
	}

	pr_debug("Initialized secure monitor\n");

	return 0;
}

/*
 * of_secure_monitor_fixup - reserve memory region for secure monitor
 *
 * We currently do not support putting the secure monitor into onchip RAM,
 * hence it runs in SDRAM and we must reserve the memory region so that we
 * won't get overwritten from the Kernel.
 * Beware: despite the name this is not secure in any way. The Kernel obeys
 * the reserve map, but only because it's nice. It could always overwrite the
 * secure monitor and hijack secure mode.
 */
static int of_secure_monitor_fixup(struct device_node *root, void *unused)
{
	unsigned long res_start, res_end;

	res_start = (unsigned long)_stext;
	res_end = (unsigned long)__bss_stop;

	of_add_reserve_entry(res_start, res_end);

	pr_debug("Reserved memory range from 0x%08lx to 0x%08lx\n", res_start, res_end);

	return 0;
}

static enum arm_security_state bootm_secure_state;

static const char * const bootm_secure_state_names[] = {
	[ARM_STATE_SECURE] = "secure",
	[ARM_STATE_NONSECURE] = "nonsecure",
	[ARM_STATE_HYP] = "hyp",
};

enum arm_security_state bootm_arm_security_state(void)
{
	return bootm_secure_state;
}

const char *bootm_arm_security_state_name(enum arm_security_state state)
{
	return bootm_secure_state_names[state];
}

static int sm_init(void)
{
	of_register_fixup(of_secure_monitor_fixup, NULL);

	globalvar_add_simple_enum("bootm.secure_state",
				  (unsigned int *)&bootm_secure_state,
				  bootm_secure_state_names,
				  ARRAY_SIZE(bootm_secure_state_names));

	return 0;
}
device_initcall(sm_init);