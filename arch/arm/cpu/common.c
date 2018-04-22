/*
 * Copyright (c) 2010 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <init.h>
#include <elf.h>
#include <linux/sizes.h>
#include <asm/system_info.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <asm-generic/memory_layout.h>
#include <asm/sections.h>
#include <asm/cache.h>
#include <debug_ll.h>

#define R_ARM_RELATIVE 23
#define R_AARCH64_RELATIVE 1027

/*
 * relocate binary to the currently running address
 */
void relocate_to_current_adr(void)
{
	unsigned long offset, offset_var;
	unsigned long __maybe_unused *dynsym, *dynend;
	void *dstart, *dend;

	/* Get offset between linked address and runtime address */
	offset = get_runtime_offset();
	offset_var = global_variable_offset();

	dstart = (void *)__rel_dyn_start + offset_var;
	dend = (void *)__rel_dyn_end + offset_var;

#if defined(CONFIG_CPU_64)
	while (dstart < dend) {
		struct elf64_rela *rel = dstart;

		if (ELF64_R_TYPE(rel->r_info) == R_AARCH64_RELATIVE) {
			unsigned long *fixup = (unsigned long *)(rel->r_offset + offset);

			*fixup = rel->r_addend + offset;
		} else {
			putc_ll('>');
			puthex_ll(rel->r_info);
			putc_ll(' ');
			puthex_ll(rel->r_offset);
			putc_ll(' ');
			puthex_ll(rel->r_addend);
			putc_ll('\n');
			panic("");
		}

		dstart += sizeof(*rel);
	}
#elif defined(CONFIG_CPU_32)
	dynsym = (void *)__dynsym_start + offset_var;
	dynend = (void *)__dynsym_end + offset_var;

	while (dstart < dend) {
		struct elf32_rel *rel = dstart;

		if (ELF32_R_TYPE(rel->r_info) == R_ARM_RELATIVE) {
			unsigned long *fixup = (unsigned long *)(rel->r_offset + offset);

			*fixup = *fixup + offset;

			rel->r_offset += offset;
		} else if (ELF32_R_TYPE(rel->r_info) == R_ARM_ABS32) {
			unsigned long r = dynsym[ELF32_R_SYM(rel->r_info) * 4 + 1];
			unsigned long *fixup = (unsigned long *)(rel->r_offset + offset);

			*fixup = *fixup + r + offset;
			rel->r_offset += offset;
		} else {
			putc_ll('>');
			puthex_ll(rel->r_info);
			putc_ll(' ');
			puthex_ll(rel->r_offset);
			putc_ll('\n');
			panic("");
		}

		dstart += sizeof(*rel);
	}

	memset(dynsym, 0, (unsigned long)dynend - (unsigned long)dynsym);
#else
#error "Architecture not specified"
#endif

	arm_early_mmu_cache_flush();
	icache_invalidate();
}

#ifdef ARM_MULTIARCH

int __cpu_architecture;

int __pure cpu_architecture(void)
{
	if(__cpu_architecture == CPU_ARCH_UNKNOWN)
		__cpu_architecture = arm_early_get_cpu_architecture();

	return __cpu_architecture;
}
#endif
