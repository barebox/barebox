#include <common.h>
#include <of.h>
#include <memory.h>

int of_add_memory_bank(struct device_node *node, bool dump, int r,
		u64 base, u64 size)
{
	static char str[6];

	sprintf(str, "ram%d", r);

	if (dump)
		pr_info("%s: %s: 0x%llx@0x%llx\n", node->name, str, size, base);

	return barebox_add_memory_bank(str, base, size);
}
