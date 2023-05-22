// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2012 Jan Luebbe <j.luebbe@pengutronix.de>, Pengutronix
/*
 * mmuinfo.c - Show MMU/cache information
 */

#include <common.h>
#include <command.h>
#include <asm/mmuinfo.h>
#include <asm/system_info.h>
#include <mmu.h>

int mmuinfo(void *addr)
{
	if (IS_ENABLED(CONFIG_CPU_V8))
		return mmuinfo_v8(addr);
	if (IS_ENABLED(CONFIG_CPU_V7) && cpu_architecture() == CPU_ARCH_ARMv7)
		return mmuinfo_v7(addr);

	return -ENOSYS;
}

static __maybe_unused int do_mmuinfo(int argc, char *argv[])
{
	unsigned long addr;

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	addr = strtoul_suffix(argv[1], NULL, 0);

	return mmuinfo((void *)addr);
}

#ifdef CONFIG_COMMAND_SUPPORT
BAREBOX_CMD_START(mmuinfo)
	.cmd            = do_mmuinfo,
	BAREBOX_CMD_DESC("show MMU/cache information of an address")
	BAREBOX_CMD_OPTS("ADDRESS")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
BAREBOX_CMD_END
#endif
