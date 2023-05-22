// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2012 Jan Luebbe <j.luebbe@pengutronix.de>, Pengutronix
/*
 * mmuinfo.c - Show MMU/cache information
 */

#include <common.h>
#include <command.h>
#include <getopt.h>
#include <asm/mmuinfo.h>
#include <asm/system_info.h>
#include <zero_page.h>
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
	int access_zero_page = -1;
	int opt;

	while ((opt = getopt(argc, argv, "zZ")) > 0) {
		switch (opt) {
		case 'z':
			access_zero_page = true;
			break;
		case 'Z':
			access_zero_page = false;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (access_zero_page >= 0) {
		if (argc - optind != 0)
			return COMMAND_ERROR_USAGE;

		if (!zero_page_remappable()) {
			pr_warn("No architecture support for zero page remap\n");
			return -ENOSYS;
		}

		if (access_zero_page)
			zero_page_access();
		else
			zero_page_faulting();

		return 0;
	}

	if (argc - optind != 1)
		return COMMAND_ERROR_USAGE;

	addr = strtoul_suffix(argv[1], NULL, 0);

	return mmuinfo((void *)addr);
}

BAREBOX_CMD_HELP_START(mmuinfo)
BAREBOX_CMD_HELP_TEXT("Show MMU/cache information using the cp15/model-specific registers.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-z",  "enable access to zero page")
BAREBOX_CMD_HELP_OPT ("-Z",  "disable access to zero page")
BAREBOX_CMD_HELP_END

#ifdef CONFIG_COMMAND_SUPPORT
BAREBOX_CMD_START(mmuinfo)
	.cmd            = do_mmuinfo,
	BAREBOX_CMD_DESC("show MMU/cache information of an address")
	BAREBOX_CMD_OPTS("[-zZ | ADDRESS]")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
	BAREBOX_CMD_HELP(cmd_mmuinfo_help)
BAREBOX_CMD_END
#endif
