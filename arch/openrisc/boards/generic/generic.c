#include <common.h>
#include <init.h>

static int openrisc_core_init(void)
{
	barebox_set_hostname("or1k");

	return 0;
}
core_initcall(openrisc_core_init);
