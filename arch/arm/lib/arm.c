#include <common.h>
#include <init.h>
#include <mem_malloc.h>

int arm_mem_malloc_init(void)
{
	mem_malloc_init((void *)(_armboot_start - CFG_MALLOC_LEN),
			(void *)_armboot_start);
	return 0;
}

core_initcall(arm_mem_malloc_init);
