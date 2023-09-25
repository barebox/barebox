// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2010 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

#include <common.h>
#include <init.h>
#include <elf.h>
#include <linux/sizes.h>
#include <asm/system_info.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <asm-generic/memory_layout.h>
#include <asm/secure.h>
#include <asm/sections.h>
#include <asm/cache.h>
#include <debug_ll.h>

/**
 * sync_caches_for_execution - synchronize caches for code execution
 *
 * Code has been modified in memory, call this before executing it.
 * This function flushes the data cache up to the point of unification
 * and invalidates the instruction cache.
 */
void sync_caches_for_execution(void)
{
	/* if caches are disabled, don't do data cache maintenance */
	if (!(get_cr() & CR_C)) {
		icache_invalidate();
		return;
	}

	/*
	 * Despite the name arm_early_mmu_cache_flush not only flushes the
	 * data cache, but also invalidates the instruction cache.
	 */
	arm_early_mmu_cache_flush();
}

#define R_ARM_RELATIVE 23
#define R_AARCH64_RELATIVE 1027

void pbl_barebox_break(void)
{
	__asm__ __volatile__ (
#ifdef CONFIG_PBL_BREAK
#ifdef CONFIG_CPU_V8
		"brk #17\n"
#else
		"bkpt #17\n"
#endif
		"nop\n"
#else
		"nop\n"
		"nop\n"
#endif
	);
}

/*
 * relocate binary to the currently running address
 */
void __prereloc relocate_to_current_adr(void)
{
	unsigned long offset;
	unsigned long __maybe_unused *dynsym, *dynend;
	void *dstart, *dend;

	/* Get offset between linked address and runtime address */
	offset = get_runtime_offset();

	/*
	 * We have yet to relocate, so using runtime_address
	 * to compute the relocated address
	 */
	dstart = runtime_address(__rel_dyn_start);
	dend = runtime_address(__rel_dyn_end);

#if defined(CONFIG_CPU_64)
	while (dstart < dend) {
		struct elf64_rela *rel = dstart;

		if (ELF64_R_TYPE(rel->r_info) == R_AARCH64_RELATIVE) {
			unsigned long *fixup = (unsigned long *)(rel->r_offset + offset);

			*fixup = rel->r_addend + offset;
			rel->r_addend += offset;
			rel->r_offset += offset;
		} else {
			putc_ll('>');
			puthex_ll(rel->r_info);
			putc_ll(' ');
			puthex_ll(rel->r_offset);
			putc_ll(' ');
			puthex_ll(rel->r_addend);
			putc_ll('\n');
			__hang();
		}

		dstart += sizeof(*rel);
	}
#elif defined(CONFIG_CPU_32)
	dynsym = runtime_address(__dynsym_start);
	dynend = runtime_address(__dynsym_end);

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
			__hang();
		}

		dstart += sizeof(*rel);
	}

	__memset(dynsym, 0, (unsigned long)dynend - (unsigned long)dynsym);
#else
#error "Architecture not specified"
#endif

	sync_caches_for_execution();
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

extern int __boot_cpu_mode;

int boot_cpu_mode(void)
{
	return __boot_cpu_mode;
}
