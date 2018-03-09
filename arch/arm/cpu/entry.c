#include <types.h>

#include <asm/cache.h>
#include <asm/barebox-arm.h>

#include "entry.h"

/*
 * Main ARM entry point. Call this with the memory region you can
 * spare for barebox. This doesn't necessarily have to be the full
 * SDRAM. The currently running binary can be inside or outside of
 * this region. TEXT_BASE can be inside or outside of this
 * region. boarddata will be preserved and can be accessed later with
 * barebox_arm_boarddata().
 *
 * -> membase + memsize
 *   STACK_SIZE              - stack
 *   16KiB, aligned to 16KiB - First level page table if early MMU support
 *                             is enabled
 *   128KiB                  - early memory space
 * -> maximum end of barebox binary
 *
 * Usually a TEXT_BASE of 1MiB below your lowest possible end of memory should
 * be fine.
 */

void NAKED __noreturn barebox_arm_entry(unsigned long membase,
					  unsigned long memsize, void *boarddata)
{
	arm_setup_stack(arm_mem_stack_top(membase, membase + memsize) - 16);
	arm_early_mmu_cache_invalidate();

	if (IS_ENABLED(CONFIG_PBL_MULTI_IMAGES))
		barebox_multi_pbl_start(membase, memsize, boarddata);
	else if (IS_ENABLED(CONFIG_PBL_SINGLE_IMAGE))
		barebox_single_pbl_start(membase, memsize, boarddata);
	else
		barebox_non_pbl_start(membase, memsize, boarddata);
}
