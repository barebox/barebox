// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2022 Ahmad Fatoum, Pengutronix

#include <common.h>
#include <of.h>
#include <command.h>
#include <getopt.h>
#include <tlv/tlv.h>

static int do_tlv(int argc, char *argv[])
{
	const char *filename;
	bool fixup = false;
	struct tlv_device *tlvdev;
	int opt;

	while ((opt = getopt(argc, argv, "f")) > 0) {
		switch (opt) {
		case 'f':
			fixup = true;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	filename = argv[optind];
	if (!filename)
		return COMMAND_ERROR_USAGE;

	tlvdev = tlv_register_device_by_path(argv[optind], NULL);
	if (IS_ERR(tlvdev))
		return PTR_ERR(tlvdev);

	if (fixup)
		return tlv_of_register_fixup(tlvdev);

	of_print_nodes(tlv_of_node(tlvdev), 0, ~0);

	tlv_free_device(tlvdev);
	return 0;
}

BAREBOX_CMD_HELP_START(tlv)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-f",  "Register device tree fixup for TLV")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(tlv)
	.cmd		= do_tlv,
	BAREBOX_CMD_DESC("handle barebox TLV")
	BAREBOX_CMD_OPTS("[-f] FILE")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_tlv_help)
BAREBOX_CMD_END
