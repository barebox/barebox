#include <common.h>
#include <init.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/ctype.h>

void __noreturn hang(void)
{
	while (1);
}

void __noreturn panic(const char *fmt, ...)
{
	while(1);
}

void __noreturn start_barebox(void)
{
	/* Should never be here in the pbl */
	hang();
}
