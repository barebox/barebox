// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <init.h>
#include <memory.h>
#include <asm-generic/memory_layout.h>
#include <asm/sections.h>

static int display_meminfo(void)
{
	ulong mstart = mem_malloc_start();
	ulong mend   = mem_malloc_end();
	ulong msize  = mend - mstart + 1;

	if (!IS_ENABLED(CONFIG_SANDBOX)) {
		pr_debug("barebox code: 0x%lx -> 0x%lx\n", (ulong)_stext, (ulong)_etext - 1);
		pr_debug("bss segment:  0x%lx -> 0x%lx\n", (ulong)__bss_start, (ulong)__bss_stop - 1);
	}
	pr_info("malloc space: 0x%08lx -> 0x%08lx (size %s)\n",
		mstart, mend, size_human_readable(msize));
	return 0;
}
late_initcall(display_meminfo);
