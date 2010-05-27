/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/**
 * @file
 * @brief tftp, rarpboot, dhcp, nfs, cdp - Boot support
 */

#include <common.h>
#include <command.h>
#include <environment.h>
#include <driver.h>
#include <net.h>
#include <fs.h>
#include <errno.h>
#include <libbb.h>

void netboot_update_env(void)
{
	struct eth_device *eth_current = eth_get_current();
	char tmp[22];
	IPaddr_t net_gateway_ip = NetOurGatewayIP;
	IPaddr_t net_ip = NetOurIP;
	IPaddr_t net_server_ip = NetServerIP;
	IPaddr_t netmask = NetOurSubnetMask;

	if (net_gateway_ip)
		dev_set_param_ip(&eth_current->dev, "gateway", net_gateway_ip);

	if (netmask)
		dev_set_param_ip(&eth_current->dev, "netmask", netmask);


	if (NetOurHostName[0])
		setenv ("hostname", NetOurHostName);

	if (NetOurRootPath[0])
		setenv ("rootpath", NetOurRootPath);

	if (net_ip)
		dev_set_param_ip(&eth_current->dev, "ipaddr", net_ip);

	if (net_server_ip)
		dev_set_param_ip(&eth_current->dev, "serverip", net_server_ip);

	if (NetOurDNSIP) {
		ip_to_string (NetOurDNSIP, tmp);
		setenv ("dnsip", tmp);
	}
#ifdef CONFIG_BOOTP_DNS2
	if (NetOurDNS2IP) {
		ip_to_string (NetOurDNS2IP, tmp);
		setenv ("dnsip2", tmp);
	}
#endif
	if (NetOurNISDomain[0])
		setenv ("domain", NetOurNISDomain);
}

#ifdef CONFIG_NET_RARP
extern void RarpRequest(void);

static int do_rarpb(struct command *cmdtp, int argc, char *argv[])
{
	int size;

	if (NetLoopInit(RARP) < 0)
		return 1;

	NetOurIP = 0;
	RarpRequest();		/* Basically same as BOOTP */

	if ((size = NetLoop()) < 0)
		return 1;

	/* NetLoop ok, update environment */
	netboot_update_env();

	return 0;
}

BAREBOX_CMD_START(rarpboot)
	.cmd		= do_rarpb,
	.usage		= "boot image via network using rarp/tftp protocol",
	BAREBOX_CMD_HELP("[loadAddress] [bootfilename]\n")
BAREBOX_CMD_END
#endif /* CONFIG_NET_RARP */

static int do_ethact(struct command *cmdtp, int argc, char *argv[])
{
	struct eth_device *edev;

	if (argc == 1) {
		edev = eth_get_current();
		if (edev)
			printf("%s%d\n", edev->dev.name, edev->dev.id);
		return 0;
	}

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	edev = eth_get_byname(argv[1]);
	if (edev)
		eth_set_current(edev);
	else {
		printf("no such net device: %s\n", argv[1]);
		return 1;
	}

	return 0;
}

static const __maybe_unused char cmd_ethact_help[] =
"Usage: ethact [ethx]\n";

BAREBOX_CMD_START(ethact)
	.cmd		= do_ethact,
	.usage		= "set current ethernet device",
	BAREBOX_CMD_HELP(cmd_ethact_help)
BAREBOX_CMD_END

