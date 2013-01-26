#include <common.h>
#include <init.h>
#include <sizes.h>
#include <asm/memory.h>

static int mem_init(void)
{
	mips_add_ram0(SZ_32M);

	return 0;
}
mem_initcall(mem_init);
