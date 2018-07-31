#include <common.h>
#include <init.h>
#include <io.h>
#include <bbu.h>

static int achilles_init(void)
{
	int pbl_index = 0;

	if (!of_machine_is_compatible("reflex,achilles"))
		return 0;

	pbl_index = readl(0xFFD06210);

	pr_debug("Current barebox instance %d\n", pbl_index);

	return 0;
}
postcore_initcall(achilles_init);
