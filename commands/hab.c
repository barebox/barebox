/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 */

#include <common.h>
#include <command.h>
#include <complete.h>
#include <fs.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/ctype.h>
#include <errno.h>
#include <hab.h>

static int do_hab(int argc, char *argv[])
{
	int opt, ret, i;
	char *srkhashfile = NULL, *srkhash = NULL;
	unsigned flags = 0;
	u8 srk[SRK_HASH_SIZE];
	int lockdown = 0, info = 0;

	while ((opt = getopt(argc, argv, "s:fpx:li")) > 0) {
		switch (opt) {
		case 's':
			srkhashfile = optarg;
			break;
		case 'f':
			flags |= IMX_SRK_HASH_FORCE;
			break;
		case 'p':
			flags |= IMX_SRK_HASH_WRITE_PERMANENT;
			break;
		case 'x':
			srkhash = optarg;
			break;
		case 'l':
			lockdown = 1;
			break;
		case 'i':
			info = 1;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (!info && !lockdown && !srkhashfile && !srkhash) {
		printf("Nothing to do\n");
		return COMMAND_ERROR_USAGE;
	}

	if (info) {
		ret = imx_hab_read_srk_hash(srk);
		if (ret)
			return ret;

		printf("Current SRK hash: ");
		for (i = 0; i < SRK_HASH_SIZE; i++)
			printf("%02x", srk[i]);
		printf("\n");

		if (imx_hab_device_locked_down())
			printf("secure mode\n");
		else
			printf("devel mode\n");

		return 0;
	}

	if (srkhashfile && srkhash) {
		printf("-s and -x options may not be given together\n");
		return COMMAND_ERROR_USAGE;
	}

	if (srkhashfile) {
		ret = imx_hab_write_srk_hash_file(srkhashfile, flags);
		if (ret)
			return ret;
	} else if (srkhash) {
		ret = imx_hab_write_srk_hash_hex(srkhash, flags);
		if (ret)
			return ret;
	}

	if (lockdown) {
		ret = imx_hab_lockdown_device(flags);
		if (ret)
			return ret;
		printf("Device successfully locked down\n");
	}

	return 0;
}

BAREBOX_CMD_HELP_START(hab)
BAREBOX_CMD_HELP_TEXT("Handle i.MX HAB (High Assurance Boot)")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_OPT ("-s <file>",  "Burn Super Root Key hash from <file>")
BAREBOX_CMD_HELP_OPT ("-x <sha256>",  "Burn Super Root Key hash from hex string")
BAREBOX_CMD_HELP_OPT ("-i",  "Print HAB info")
BAREBOX_CMD_HELP_OPT ("-f",  "Force. Write even when a key is already written")
BAREBOX_CMD_HELP_OPT ("-l",  "Lockdown device. Dangerous! After executing only signed images can be booted")
BAREBOX_CMD_HELP_OPT ("-p",  "Permanent. Really burn fuses. Be careful!")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(hab)
	.cmd		= do_hab,
	BAREBOX_CMD_DESC("Handle i.MX HAB")
	BAREBOX_CMD_OPTS("sxfp")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_hab_help)
BAREBOX_CMD_END
