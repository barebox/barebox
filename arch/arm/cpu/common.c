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

void __prereloc relocate_to_current_adr(void)
{
	relocate_image(get_runtime_offset(),
		       runtime_address(__rel_dyn_start),
		       runtime_address(__rel_dyn_end),
		       runtime_address(__dynsym_start),
		       runtime_address(__dynsym_end));

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

void print_pbl_mem_layout(ulong membase, ulong endmem, ulong barebox_base)
{
	printf("endmem                = 0x%08lx\n", endmem);
	if (OPTEE_SIZE)
		printf("arm_mem_optee         = 0x%08lx+0x%08x\n", arm_mem_optee(endmem),
		       OPTEE_SIZE);
	printf("arm_mem_scratch       = 0x%08lx+0x%08lx\n",
	       arm_mem_scratch(endmem),
	       arm_mem_barebox_image_end(endmem) - arm_mem_scratch(endmem));
	printf("arm_mem_stack         = 0x%08lx+0x%08x\n",
	       arm_mem_stack(endmem), STACK_SIZE);
	if (IS_ENABLED(CONFIG_STACK_GUARD_PAGE))
		printf("arm_mem_guard_page    = 0x%08lx+0x%08x\n",
		       arm_mem_guard_page(endmem), PAGE_SIZE);
	printf("arm_mem_ttb           = 0x%08lx+0x%08x\n",
	       arm_mem_ttb(endmem), ARM_EARLY_PAGETABLE_SIZE);
#ifdef CONFIG_FS_PSTORE_RAMOOPS
	printf("arm_mem_ramoops       = 0x%08lx+0x%08x\n",
	       arm_mem_ramoops(endmem), CONFIG_FS_PSTORE_RAMOOPS_SIZE);
#endif
	printf("arm_mem_barebox_image = 0x%08lx+0x%08lx\n",
	       barebox_base, arm_mem_barebox_image_end(endmem) - barebox_base);
	printf("arm_mem_early_malloc  = 0x%08lx+0x%08x\n",
	       barebox_base - ARM_MEM_EARLY_MALLOC_SIZE, ARM_MEM_EARLY_MALLOC_SIZE);
	printf("membase               = 0x%08lx+0x%08lx\n",
	       membase, endmem - membase);
}
