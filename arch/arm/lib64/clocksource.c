// SPDX-License-Identifier: GPL-2.0-only

#include <debug_ll.h>
#include <asm/hardware/arm_architected_timer.h>

__attribute__((constructor)) static void init_arch_clock(void)
{
	if (arm_arch_timer_init(0))
		puts_ll("Failed to setup architected timer\n");
}
