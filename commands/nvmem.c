// SPDX-License-Identifier: GPL-2.0
// SPDX-FileCopyrightText: © 2021 Ahmad Fatoum, Pengutronix

#include <common.h>
#include <command.h>
#include <linux/nvmem-consumer.h>

static int do_nvmem(int argc, char *argv[])
{
	nvmem_devices_print();

	return 0;
}

BAREBOX_CMD_HELP_START(nvmem)
BAREBOX_CMD_HELP_TEXT("Usage: nvmem")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(nvmem)
	.cmd		= do_nvmem,
	BAREBOX_CMD_DESC("list nvmem devices")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_nvmem_help)
BAREBOX_CMD_END
