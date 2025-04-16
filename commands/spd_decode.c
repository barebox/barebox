// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: © 2015 Alexander Smirnov <alllecs@yandex.ru>

/*
 * This program is decoding and printing SPD contents
 * in human readable format
 * As an argument program, you must specify the file name.
 */

#include <common.h>
#include <command.h>
#include <libfile.h>
#include <malloc.h>
#include <ddr_spd.h>

static int do_spd_decode(int argc, char *argv[])
{
	int ret;
	void *data;

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	ret = read_file_2(argv[1], NULL, &data, 256);
	if (ret && ret != -EFBIG) {
		printf("unable to read %s: %pe\n", argv[1], ERR_PTR(ret));
		return COMMAND_ERROR;
	}

	printf("Decoding EEPROM: %s\n\n", argv[1]);
	ddr_spd_print(data);

	free(data);

	return 0;
}

BAREBOX_CMD_HELP_START(spd_decode)
BAREBOX_CMD_HELP_TEXT("Decode a SPD EEPROM and print contents in human readable form")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(spd_decode)
	.cmd	= do_spd_decode,
	BAREBOX_CMD_DESC("Decode SPD EEPROM")
	BAREBOX_CMD_OPTS("FILE")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_spd_decode_help)
BAREBOX_CMD_END
