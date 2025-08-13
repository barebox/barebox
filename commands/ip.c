// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2025 Oleksij Rempel <o.rempel@pengutronix.de>

#include <common.h>
#include <command.h>
#include <net.h>
#include <linux/list.h>
#include <complete.h>

/* Function to display network links (`ip l`) */
static int do_ip_link(int argc, char *argv[])
{
	struct eth_device *edev;

	for_each_netdev(edev) {
		const char *phy_status = edev->phydev ?
			(edev->phydev->link ? ",LOWER_UP" : ",NO-CARRIER") : "";

		printf("%s: <%s%s>\n",
			   eth_name(edev),
			   edev->active ? "UP" : "DOWN",
			   phy_status);

		printf("	link/ether %pM brd ff:ff:ff:ff:ff:ff\n",
		       edev->ethaddr);
	}

	return 0;
}

/* Function to display network addresses (`ip a`) */
static int do_ip_addr(int argc, char *argv[])
{
	struct eth_device *edev;

	for_each_netdev(edev) {
		IPaddr_t ipaddr = net_get_ip(edev);
		int prefix_length = netmask_to_prefix(edev->netmask);
		const char *phy_status = edev->phydev ?
			(edev->phydev->link ? ",LOWER_UP" : ",NO-CARRIER") : "";

		printf("%s: <%s%s>\n", eth_name(edev),
		       edev->active ? "UP" : "DOWN", phy_status);

		printf("	link/ether %pM brd ff:ff:ff:ff:ff:ff\n",
		       edev->ethaddr);

		if (ipaddr) {
			printf("	inet %pI4/%d scope global src %pI4\n",
				   &ipaddr, prefix_length, &ipaddr);
		}
	}

	return 0;
}

/* Function to display network routes (`ip r`) */
static int do_ip_route(int argc, char *argv[])
{
	const char *domainname = net_get_domainname();
	IPaddr_t nameserver = net_get_nameserver();
	const char *serverip = net_get_server();
	IPaddr_t gateway = net_get_gateway();
	struct eth_device *edev;

	for_each_netdev(edev) {
		int prefix_length = netmask_to_prefix(edev->netmask);
		IPaddr_t ipaddr = net_get_ip(edev);
		const char *proto;

		/* Skip interfaces without an IP address */
		if (!ipaddr)
			continue;

		/* Determine protocol: DHCP or Static */
		if (edev->global_mode == ETH_MODE_DHCP)
			proto = "dhcp";
		else
			proto = "static";

		/* Show default gateway */
		if (gateway) {
			printf("default via %pI4 dev %s proto %s src %pI4\n",
				   &gateway, eth_name(edev), proto, &ipaddr);
		}

		/* Show per-interface network route */
		if (prefix_length > 0) {
			printf("%pI4/%d dev %s proto kernel scope link src %pI4\n",
				   &ipaddr, prefix_length, eth_name(edev),
				   &ipaddr);
		}
	}

	/* Show boot server */
	if (serverip && *serverip)
		printf("serverip %s\n", serverip);

	/* Show DNS server */
	if (nameserver)
		printf("nameserver %pI4\n", &nameserver);

	/* Show search domain */
	if (domainname && *domainname)
		printf("search %s\n", domainname);

	return 0;
}

/* Main IP command dispatcher */
static int do_ip(int argc, char *argv[])
{
	size_t arglen;

	if (argc < 2)
		return do_ip_addr(argc, argv); /* Default: `ip a` */

	arglen = strlen(argv[1]);

	if ((arglen <= strlen("link")) && !strncmp(argv[1], "link", arglen))
		return do_ip_link(argc, argv);
	else if ((arglen <= strlen("address")) && !strncmp(argv[1], "address", arglen))
		return do_ip_addr(argc, argv);
	else if ((arglen <= strlen("route")) && !strncmp(argv[1], "route", arglen))
		return do_ip_route(argc, argv);

	return COMMAND_ERROR_USAGE;
}

BAREBOX_CMD_HELP_START(ip)
BAREBOX_CMD_HELP_TEXT("Show network interfaces, addresses, and routes")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("l, link", "Show only network links (interface state and MAC)")
BAREBOX_CMD_HELP_OPT("a, address", "Show only network addresses (default behavior)")
BAREBOX_CMD_HELP_OPT("r, route", "Show routing information (default gateway, network routes, server IP, DNS)")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(ip)
	.cmd		= do_ip,
	BAREBOX_CMD_DESC("Show network interfaces, addresses, and routes")
	BAREBOX_CMD_OPTS("[l|link] | [a|addr] | [r|route]")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_COMPLETE(empty_complete)
	BAREBOX_CMD_HELP(cmd_ip_help)
BAREBOX_CMD_END
