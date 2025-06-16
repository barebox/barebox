// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* tftp.c - (up)load tftp files */

#include <common.h>
#include <command.h>
#include <linux/stat.h>
#include <getopt.h>
#include <fcntl.h>
#include <libgen.h>
#include <fs.h>
#include <net.h>
#include <libbb.h>
#include <libfile.h>

#define TFTP_MOUNT_PATH	"/.tftp_tmp_path"

static int do_tftpb(int argc, char *argv[])
{
	const char *source, *dest;
	char *freep;
	int opt;
	int tftp_push = 0;
	int port = -1;
	int ret;
	IPaddr_t ip;
	char ip4_str[sizeof("255.255.255.255")];
	char mount_opts[sizeof("port=12345")];

	while ((opt = getopt(argc, argv, "pP:")) > 0) {
		switch(opt) {
		case 'p':
			tftp_push = 1;
			break;
		case 'P':
			port = simple_strtoul(optarg, NULL, 0);
			if (port <= 0 || port > 0xffff) {
				pr_err("invalid port '%s'\n", optarg);
				return COMMAND_ERROR_USAGE;
			}
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (argc <= optind)
		return COMMAND_ERROR_USAGE;

	source = argv[optind++];

	if (argc == optind)
		dest = kbasename(source);
	else
		dest = argv[optind];

	if (tftp_push)
		dest = freep = xasprintf("%s/%s", TFTP_MOUNT_PATH, dest);
	else
		source = freep = xasprintf("%s/%s", TFTP_MOUNT_PATH, source);

	ret = make_directory(TFTP_MOUNT_PATH);
	if (ret)
		goto err_free;

	ip = net_get_serverip();
	sprintf(ip4_str, "%pI4", &ip);

	if (port >= 0)
		sprintf(mount_opts, "port=%u", port);
	else
		mount_opts[0] = '\0';

	ret = mount(ip4_str, "tftp", TFTP_MOUNT_PATH, mount_opts);
	if (ret)
		goto err_rmdir;

	debug("%s: %s -> %s\n", __func__, source, dest);

	ret = copy_file(source, dest, COPY_FILE_VERBOSE);

	umount(TFTP_MOUNT_PATH);

err_rmdir:
	rmdir(TFTP_MOUNT_PATH);

err_free:
	free(freep);

	return ret;
}

BAREBOX_CMD_HELP_START(tftp)
BAREBOX_CMD_HELP_TEXT("Load (or save) a file via TFTP. SOURCE is a path on server,")
BAREBOX_CMD_HELP_TEXT("server address is taken from the environment (ethX.serverip).")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-p", "push to TFTP server")
BAREBOX_CMD_HELP_OPT ("-P PORT", "tftp server port number")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(tftp)
	.cmd		= do_tftpb,
	BAREBOX_CMD_DESC("load (or save) a file using TFTP")
	BAREBOX_CMD_OPTS("[-p] [-P <port>] SOURCE [DEST]")
	BAREBOX_CMD_GROUP(CMD_GRP_NET)
	BAREBOX_CMD_HELP(cmd_tftp_help)
BAREBOX_CMD_END
