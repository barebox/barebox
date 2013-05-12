#include <common.h>
#include <of.h>
#include <memory.h>

void of_add_memory_bank(struct device_node *node, bool dump, int r,
		u64 base, u64 size)
{
	static char str[6];

	sprintf(str, "ram%d", r);
	barebox_add_memory_bank(str, base, size);

	if (dump)
		pr_info("%s: %s: 0x%llx@0x%llx\n", node->name, str, size, base);
}
