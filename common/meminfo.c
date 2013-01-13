#include <common.h>
#include <init.h>
#include <memory.h>
#include <asm-generic/memory_layout.h>
#include <asm-generic/sections.h>

static int display_meminfo(void)
{
	ulong mstart = mem_malloc_start();
	ulong mend   = mem_malloc_end();
	ulong msize  = mend - mstart + 1;

	debug("barebox code: 0x%p -> 0x%p\n", _stext, _etext - 1);
	debug("bss segment:  0x%p -> 0x%p\n", __bss_start, __bss_stop - 1);
	printf("malloc space: 0x%08lx -> 0x%08lx (size %s)\n",
		mstart, mend, size_human_readable(msize));
#ifdef CONFIG_ARM
	printf("stack space:  0x%08x -> 0x%08x (size %s)\n",
		STACK_BASE, STACK_BASE + STACK_SIZE - 1,
		size_human_readable(STACK_SIZE));
#endif
	return 0;
}
late_initcall(display_meminfo);
