/*
 * Copyright (C) 2015 PHYTEC Messtechnik GmbH,
 * Author: Wadim Egorov <w.egorov@phytec.de>
 *
 * Based on work of Sascha Hauer <s.hauer@pengutronix.de>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include <command.h>
#include <complete.h>
#include <environment.h>
#include <getopt.h>
#include <dhcp.h>

static int do_dhcp(int argc, char *argv[])
{
	int ret, opt;
	int retries = DHCP_DEFAULT_RETRY;
	struct dhcp_req_param dhcp_param;

	memset(&dhcp_param, 0, sizeof(struct dhcp_req_param));
	getenv_uint("global.dhcp.retries", &retries);

	while ((opt = getopt(argc, argv, "H:v:c:u:U:r:")) > 0) {
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
			retries = simple_strtoul(optarg, NULL, 10);
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (!retries) {
		printf("retries is set to zero, set it to %d\n", DHCP_DEFAULT_RETRY);
		retries = DHCP_DEFAULT_RETRY;
	}

	ret = dhcp(retries, &dhcp_param);

	return ret;
}

BAREBOX_CMD_HELP_START(dhcp)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-H HOSTNAME", "hostname to send to the DHCP server")
BAREBOX_CMD_HELP_OPT("-v ID\t", "DHCP Vendor ID (code 60) submitted in DHCP requests")
BAREBOX_CMD_HELP_OPT("-c ID\t", "DHCP Client ID (code 61) submitted in DHCP requests")
BAREBOX_CMD_HELP_OPT("-u UUID\t", "DHCP Client UUID (code 97) submitted in DHCP requests")
BAREBOX_CMD_HELP_OPT("-U CLASS", "DHCP User class (code 77) submitted in DHCP requests")
BAREBOX_CMD_HELP_OPT("-r RETRY", "retry limit (default 20)");
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(dhcp)
	.cmd		= do_dhcp,
	BAREBOX_CMD_DESC("DHCP client to obtain IP or boot params")
	BAREBOX_CMD_OPTS("[-HvcuUr]")
	BAREBOX_CMD_GROUP(CMD_GRP_NET)
	BAREBOX_CMD_HELP(cmd_dhcp_help)
	BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END
