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

/*
 * It can be hard to convince GCC to not use old stack pointer after
 * we modify it with arm_setup_stack() on ARM64, so we implement the
 * low level details in assembly
 */
void __noreturn __barebox_arm_entry(unsigned long membase,
				    unsigned long memsize,
				    void *boarddata,
				    unsigned long sp);

void NAKED __noreturn barebox_arm_entry(unsigned long membase,
					unsigned long memsize, void *boarddata)
{
	__barebox_arm_entry(membase, memsize, boarddata,
			    arm_mem_stack_top(membase, membase + memsize));
}
