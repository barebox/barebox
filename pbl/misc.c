#include <common.h>
#include <init.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/ctype.h>

void __noreturn panic(const char *fmt, ...)
{
	while(1);
}

void start_barebox(void)
{
}
