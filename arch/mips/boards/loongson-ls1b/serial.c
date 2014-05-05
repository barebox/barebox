#include <common.h>
#include <init.h>

static int console_init(void)
{
	barebox_set_hostname("ls1b");

	return 0;
}
console_initcall(console_init);
