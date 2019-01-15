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

#include <command.h>
#include <common.h>
#include <getopt.h>
#include <complete.h>
#include <environment.h>
#include <net.h>

static int do_ip_route_get(int argc, char *argv[])
{
	struct eth_device *edev;
	IPaddr_t ip, gw = 0;
	const char *variable = NULL;
	int opt, ret;
	bool bootarg = false;

	while ((opt = getopt(argc, argv, "b")) > 0) {
		switch (opt) {
		case 'b':
			bootarg = true;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (argc == optind)
		return COMMAND_ERROR_USAGE;

	if (argc == optind + 2)
		variable = argv[optind + 1];

	ret = resolv(argv[optind], &ip);
	if (ret) {
		printf("Cannot convert \"%s\" into a IP address: %s\n",
		       argv[optind], strerror(-ret));
		return 1;
	}

	edev = net_route(ip);
	if (!edev) {
		gw = net_get_gateway();
		if (gw)
			edev = net_route(gw);
	}

	if (!edev) {
		printf("IP %pI4 is not reachable\n", &ip);
		return 1;
	}

	if (variable) {
		if (bootarg)
			setenv(variable, edev->bootarg);
		else
			setenv(variable, edev->devname);
		return 0;
	}

	if (bootarg) {
		printf("%s\n", edev->bootarg);
	} else {
		if (gw)
			printf("%pI4 via %pI4 dev %s\n", &ip, &gw,
			       edev->devname);
		else
			printf("%pI4 dev %s\n", &ip, edev->devname);
	}

	return 0;
}

BAREBOX_CMD_HELP_START(ip_route_get)
BAREBOX_CMD_HELP_TEXT("get ethernet device used to reach given IP address")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-b", "Instead of ethernet device, show linux bootargs for that device")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(ip_route_get)
	.cmd = do_ip_route_get,
	BAREBOX_CMD_DESC("get ethernet device name for an IP address")
	BAREBOX_CMD_OPTS("[-b] <IP> [variable]")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_COMPLETE(empty_complete)
	BAREBOX_CMD_HELP(cmd_ip_route_get_help)
BAREBOX_CMD_END
