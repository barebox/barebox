#include <common.h>
#include <init.h>
#include <mem_malloc.h>
#include <asm/u-boot-arm.h>
#include <asm-generic/memory_layout.h>
#include <reloc.h>

int arm_mem_malloc_init(void)
{
	mem_malloc_init((void *)MALLOC_BASE,
			(void *)(MALLOC_BASE + MALLOC_SIZE));
	return 0;
}

core_initcall(arm_mem_malloc_init);
