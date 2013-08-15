#include <common.h>
#include <init.h>
#include <sizes.h>
#include <asm/memory.h>

static int mem_init(void)
{
	barebox_set_model("Netgear wg102");
	barebox_set_hostname("wg102");

	mips_add_ram0(SZ_16M);
	return 0;
}
mem_initcall(mem_init);
