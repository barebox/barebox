// SPDX-License-Identifier: GPL-2.0-or-later

#include <asm/cache.h>

void sync_caches_for_execution(void)
{
	flush_dcache();
	invalidate_icache();
}
