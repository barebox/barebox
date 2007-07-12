#include <common.h>
#include <init.h>
#include <mem_malloc.h>
#include <asm/u-boot-arm.h>
#include <reloc.h>

int arm_mem_malloc_init(void)
{
	mem_malloc_init((void *)(_u_boot_start - CFG_MALLOC_LEN),
			(void *)_u_boot_start);
	return 0;
}

core_initcall(arm_mem_malloc_init);
