// SPDX-License-Identifier: GPL-2.0

#include <mach/entry.h>
#include <debug_ll.h>

/*
 * barebox-dt-2nd.img expects being loaded at an offset, so the
 * stack can grow down from entry point. The STM32MP TF-A default
 * is to not have an offset. This stm32mp specific entry points
 * avoids this issue by setting up a 64 byte stack after end of
 * barebox and by asking the memory controller about RAM size
 * instead of parsing it out of the DT.
 *
 * When using OP-TEE, ensure CONFIG_OPTEE_SIZE is appopriately set.
 */

ENTRY_FUNCTION(start_stm32mp_bl33, r0, r1, r2)
{
	stm32mp_cpu_lowlevel_init();

	putc_ll('>');

	stm32mp1_barebox_entry((void *)r2);
}
