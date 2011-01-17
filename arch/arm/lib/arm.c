#include <common.h>
#include <init.h>
#include <mem_malloc.h>
#include <asm/barebox-arm.h>
#include <asm-generic/memory_layout.h>

static int arm_mem_malloc_init(void)
{
	mem_malloc_init((void *)MALLOC_BASE,
			(void *)(MALLOC_BASE + MALLOC_SIZE));
	return 0;
}

core_initcall(arm_mem_malloc_init);
