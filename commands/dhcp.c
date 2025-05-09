// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2015 PHYTEC Messtechnik GmbH

/*
 * Author: Wadim Egorov <w.egorov@phytec.de>
 * Based on work of Sascha Hauer <s.hauer@pengutronix.de>.
 */

#include <common.h>
#include <command.h>
#include <complete.h>
#include <environment.h>
#include <getopt.h>
#include <dhcp.h>
#include <net.h>

static int do_dhcp(int argc, char *argv[])
{
	int ret, opt;
	struct dhcp_req_param dhcp_param = {};
	struct eth_device *edev;
	const char *edevname;

	while ((opt = getopt(argc, argv, "H:v:c:u:U:r:o:")) > 0) {
		switch (opt) {
		case 'H':
			dhcp_param.hostname = optarg;
			break;
		case 'v':
			dhcp_param.vendor_id = optarg;
			break;
		case 'c':
			dhcp_param.client_id = optarg;
			break;
		case 'u':
			dhcp_param.client_uuid = optarg;
			break;
		case 'U':
			dhcp_param.user_class = optarg;
			break;
		case 'r':
			dhcp_param.retries = simple_strtoul(optarg, NULL, 10);
			break;
		case 'o':
			dhcp_param.option224 = optarg;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	switch (argc - optind) {
	case 0:
		edevname = "eth0";
		puts("Defaulting to eth0\n");
		break;
	case 1:
		edevname = argv[optind];
		break;
	default:
		return COMMAND_ERROR_USAGE;
	}

	edev = eth_get_byname(edevname);
	if (!edev) {
		printf("No such network device: %s\n", edevname);
		return 1;
	}

	ret = dhcp(edev, &dhcp_param);

	return ret;
}

BAREBOX_CMD_HELP_START(dhcp)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-H HOSTNAME", "hostname to send to the DHCP server")
BAREBOX_CMD_HELP_OPT("-v ID\t", "DHCP Vendor ID (code 60) submitted in DHCP requests")
BAREBOX_CMD_HELP_OPT("-c ID\t", "DHCP Client ID (code 61) submitted in DHCP requests")
BAREBOX_CMD_HELP_OPT("-u UUID\t", "DHCP Client UUID (code 97) submitted in DHCP requests")
BAREBOX_CMD_HELP_OPT("-U CLASS", "DHCP User class (code 77) submitted in DHCP requests")
BAREBOX_CMD_HELP_OPT("-r RETRY", "retry limit (default 20)")
BAREBOX_CMD_HELP_OPT("-o PRIVATE DATA", "private data (code 224) submitted in DHCP requests");
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(dhcp)
	.cmd		= do_dhcp,
	BAREBOX_CMD_DESC("DHCP client to obtain IP or boot params")
	BAREBOX_CMD_OPTS("[-HvcuUro] [device]")
	BAREBOX_CMD_GROUP(CMD_GRP_NET)
	BAREBOX_CMD_HELP(cmd_dhcp_help)
	BAREBOX_CMD_COMPLETE(eth_complete)
BAREBOX_CMD_END
