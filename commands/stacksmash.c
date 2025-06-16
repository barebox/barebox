/* SPDX-License-Identifier: GPL-2.0-only */

#include <common.h>
#include <command.h>
#include <complete.h>
#include <linux/compiler.h>
#include <string.h>

#define NO_FORTIFY_SOURCE "We want to test stack protector, not FORTIFY_SOURCE"

static noinline void stack_overflow_frame(void)
{
	volatile int length = 512;
	char a[128] = {};

	/*
	 * In order to avoid having the compiler optimize away the stack smashing
	 * we need to do a little something here.
	 */
	OPTIMIZER_HIDE_VAR(length);

	unsafe_memset(a, 0xa5, length, NO_FORTIFY_SOURCE);

	printf("We have smashed our stack as this should not exceed 128: sizeof(a) = %zu\n",
	       unsafe_strlen(a, NO_FORTIFY_SOURCE));
}

static noinline void stack_overflow_region(u64 i)
{
	volatile char a[1024] = {};

	if (ctrlc())
		return;

	RELOC_HIDE(&a, 0);

	stack_overflow_region(0);

	printf("%*ph", 1024, a);
}

static int do_stacksmash(int argc, char *argv[])
{
	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	if (!strcmp(argv[1], "frame"))
		stack_overflow_frame();
	else if (!strcmp(argv[1], "region"))
		stack_overflow_region(0);

	panic("Stack smashing of %s not caught\n", argv[1]);
}
BAREBOX_CMD_START(stacksmash)
	.cmd = do_stacksmash,
	BAREBOX_CMD_DESC("Run stack smashing tests")
	BAREBOX_CMD_OPTS("[frame | region]")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END
