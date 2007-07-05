#include <init.h>
#include <mem_malloc.h>

int arm_mem_alloc_init(void)
{
	mem_alloc_init(_armboot_start - CFG_MALLOC_LEN,
			_armboot_start);
}

core_initcall(arm_mem_alloc_init);
