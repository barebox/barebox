// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2022 Ahmad Fatoum

#include <common.h>
#include <command.h>
#include <getopt.h>
#include <video/mipi_dbi.h>
#include <video/mipi_display.h>

static int mipi_dbi_command_show(struct mipi_dbi *dbi, int cmd)
{
	u8 val[4];
	int ret;
	size_t len;

	if (!mipi_dbi_command_is_read(dbi, cmd))
		return -EACCES;

	len = mipi_dbi_command_read_len(cmd);

	printf("%02x: ", cmd);
	ret = mipi_dbi_command_buf(dbi, cmd, val, len);
	if (ret) {
		printf("XX\n");
		return ret;
	}
	printf("%*phN\n", (int)len, val);

	return 0;
}

static int do_mipi_dbi(int argc, char *argv[])
{
	struct mipi_dbi *dbi;
	int opt, ret, i;
	bool write = false;
	u8 cmd, val[4];

	dbi = list_first_entry_or_null(&mipi_dbi_list, struct mipi_dbi, list);

	while ((opt = getopt(argc, argv, "wld:")) > 0) {
		struct mipi_dbi *tmp;

		switch (opt) {
		case 'w':
			write = true;
			break;
		case 'l':
			list_for_each_entry(tmp, &mipi_dbi_list, list)
				printf("%s\n", mipi_dbi_name(tmp));
			return 0;
		case 'd':
			dbi = NULL;
			list_for_each_entry(tmp, &mipi_dbi_list, list) {
				if (!strcmp(optarg, mipi_dbi_name(tmp))) {
					dbi = tmp;
					break;
				}
			}
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (!dbi)
		return -ENODEV;

	if (optind == argc) {
		for (cmd = 0; cmd < 255; cmd++)
			mipi_dbi_command_show(dbi, cmd);
		return 0;
	}

	ret = kstrtou8(argv[optind++], 16, &cmd);
	if (ret < 0)
		return ret;

	if (optind == argc && !write)
		return mipi_dbi_command_show(dbi, cmd);

	if (argc > 6) {
		printf("Error: can only write up to 4 byte at once!\n");
		return -EOVERFLOW;
	}

	for (i = 0; i + optind < argc; i++) {
		ret = kstrtou8(argv[optind + i], 16, &val[i]);
		if (ret < 0)
			return ret;
	}

	return mipi_dbi_command_buf(dbi, cmd, val, argc - optind);
}

BAREBOX_CMD_HELP_START(mipi_dbi)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-l\t",  "list all MIPI DBI devices")
BAREBOX_CMD_HELP_OPT ("-d DEVICE",  "select specific device (default is first registered)")
BAREBOX_CMD_HELP_OPT ("-w",  "issue write command")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(mipi_dbi)
	.cmd		= do_mipi_dbi,
	BAREBOX_CMD_DESC("write/read from MIPI DBI SPI device")
	BAREBOX_CMD_OPTS("[-wld] [REG] [DATA...]")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_mipi_dbi_help)
BAREBOX_CMD_END
