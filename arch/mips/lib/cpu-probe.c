// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Processor capabilities determination functions.
 *
 * Copyright (C) xxxx  the Anonymous
 * Copyright (C) 1994 - 2006 Ralf Baechle
 * Copyright (C) 2003, 2004  Maciej W. Rozycki
 * Copyright (C) 2001, 2004  MIPS Inc.
 */
#include <common.h>
#include <asm/mipsregs.h>
#include <asm/cache.h>
#include <asm/cpu-features.h>
#include <asm/cpu-info.h>
#include <asm/cpu.h>
#include <memory.h>
#include <asm-generic/memory_layout.h>
#include <init.h>

const char *__cpu_name = "unknown";
struct cpuinfo_mips cpu_data[1];

static char unknown_isa[] = KERN_ERR \
	"Unsupported ISA type, c0.config0: %d.";

static inline unsigned int decode_config0(struct cpuinfo_mips *c)
{
	unsigned int config0;
	int isa;

	config0 = read_c0_config();

	if (((config0 & MIPS_CONF_MT) >> 7) == 1)
		c->options |= MIPS_CPU_TLB;
	isa = (config0 & MIPS_CONF_AT) >> 13;
	switch (isa) {
	case 0:
		switch ((config0 & MIPS_CONF_AR) >> 10) {
		case 0:
			c->isa_level = MIPS_CPU_ISA_M32R1;
			break;
		case 1:
			c->isa_level = MIPS_CPU_ISA_M32R2;
			break;
		default:
			goto unknown;
		}
		break;
	case 2:
		switch ((config0 & MIPS_CONF_AR) >> 10) {
		case 0:
			c->isa_level = MIPS_CPU_ISA_M64R1;
			break;
		case 1:
			c->isa_level = MIPS_CPU_ISA_M64R2;
			break;
		default:
			goto unknown;
		}
		break;
	default:
		goto unknown;
	}

	return config0 & MIPS_CONF_M;

unknown:
	panic(unknown_isa, config0);
}

static void decode_configs(struct cpuinfo_mips *c)
{
	int ok;

	/* MIPS32 or MIPS64 compliant CPU.  */
	c->options = MIPS_CPU_4KEX | MIPS_CPU_4K_CACHE | MIPS_CPU_COUNTER |
	             MIPS_CPU_DIVEC | MIPS_CPU_LLSC | MIPS_CPU_MCHECK;

	ok = decode_config0(c);			/* Read Config registers.  */
	BUG_ON(!ok);				/* Arch spec violation!  */
}

static inline void cpu_probe_legacy(struct cpuinfo_mips *c)
{
	switch (c->processor_id & PRID_IMP_MASK) {
	case PRID_IMP_GS232:
		decode_configs(c);

		c->cputype = CPU_GS232;

		switch (c->processor_id & PRID_REV_MASK) {
		case PRID_REV_LOONGSON1B:
			__cpu_name = "Loongson 1B";
			break;
		}

		break;
	}
}

static inline void cpu_probe_mips(struct cpuinfo_mips *c)
{
	decode_configs(c);
	switch (c->processor_id & 0xff00) {
	case PRID_IMP_QEMU_GENERIC:
		c->cputype = CPU_QEMU_GENERIC;
		__cpu_name = "MIPS GENERIC QEMU";
		break;
	case PRID_IMP_4KC:
		c->cputype = CPU_4KC;
		__cpu_name = "MIPS 4Kc";
		break;
	case PRID_IMP_4KEC:
	case PRID_IMP_4KECR2:
		c->cputype = CPU_4KEC;
		__cpu_name = "MIPS 4KEc";
		break;
	case PRID_IMP_4KSC:
	case PRID_IMP_4KSD:
		c->cputype = CPU_4KSC;
		__cpu_name = "MIPS 4KSc";
		break;
	case PRID_IMP_5KC:
		c->cputype = CPU_5KC;
		__cpu_name = "MIPS 5Kc";
		break;
	case PRID_IMP_5KE:
		c->cputype = CPU_5KE;
		__cpu_name = "MIPS 5KE";
		break;
	case PRID_IMP_20KC:
		c->cputype = CPU_20KC;
		__cpu_name = "MIPS 20Kc";
		break;
	case PRID_IMP_24K:
		c->cputype = CPU_24K;
		__cpu_name = "MIPS 24Kc";
		break;
	case PRID_IMP_24KE:
		c->cputype = CPU_24K;
		__cpu_name = "MIPS 24KEc";
		break;
	case PRID_IMP_25KF:
		c->cputype = CPU_25KF;
		__cpu_name = "MIPS 25Kc";
		break;
	case PRID_IMP_34K:
		c->cputype = CPU_34K;
		__cpu_name = "MIPS 34Kc";
		break;
	case PRID_IMP_74K:
		c->cputype = CPU_74K;
		__cpu_name = "MIPS 74Kc";
		break;
	case PRID_IMP_M14KC:
		c->cputype = CPU_M14KC;
		__cpu_name = "MIPS M14Kc";
		break;
	case PRID_IMP_M14KEC:
		c->cputype = CPU_M14KEC;
		__cpu_name = "MIPS M14KEc";
		break;
	case PRID_IMP_1004K:
		c->cputype = CPU_1004K;
		__cpu_name = "MIPS 1004Kc";
		break;
	case PRID_IMP_1074K:
		c->cputype = CPU_1074K;
		__cpu_name = "MIPS 1074Kc";
		break;
	}
}

static inline void cpu_probe_broadcom(struct cpuinfo_mips *c)
{
	decode_configs(c);
	switch (c->processor_id & 0xff00) {
	case PRID_IMP_BMIPS3300:
		c->cputype = CPU_BMIPS3300;
		__cpu_name = "Broadcom BMIPS3300";
		break;
	}
}

static inline void cpu_probe_ingenic(struct cpuinfo_mips *c)
{
	decode_configs(c);
	/* JZRISC does not implement the CP0 counter. */
	c->options &= ~MIPS_CPU_COUNTER;
	switch (c->processor_id & 0xff00) {
	case PRID_IMP_JZRISC:
		c->cputype = CPU_JZRISC;
		__cpu_name = "Ingenic JZRISC";
		break;
	default:
		panic("Unknown Ingenic Processor ID!");
		break;
	}
}

void cpu_probe(void)
{
	struct cpuinfo_mips *c = &current_cpu_data;

	c->processor_id	= PRID_IMP_UNKNOWN;
	c->fpu_id	= FPIR_IMP_NONE;
	c->cputype	= CPU_UNKNOWN;

	c->processor_id = read_c0_prid();
	switch (c->processor_id & 0xff0000) {
	case PRID_COMP_LEGACY:
		cpu_probe_legacy(c);
		break;
	case PRID_COMP_MIPS:
		cpu_probe_mips(c);
		break;
	case PRID_COMP_BROADCOM:
		cpu_probe_broadcom(c);
		break;
	case PRID_COMP_INGENIC:
	case PRID_COMP_INGENIC2:
		cpu_probe_ingenic(c);
		break;
	}

	if (cpu_has_4k_cache)
		r4k_cache_init();
}

unsigned long mips_stack_top;

static int mips_request_stack(void)
{
	if (!request_barebox_region("stack", mips_stack_top - STACK_SIZE, STACK_SIZE))
		pr_err("Error: Cannot request SDRAM region for stack\n");

	return 0;
}
coredevice_initcall(mips_request_stack);
