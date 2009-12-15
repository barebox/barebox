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
#include <fcntl.h>
#include <errno.h>
#include <libbb.h>
#include <libgen.h>

static int netboot_common (proto_t, cmd_tbl_t *, int , char *[]);

void netboot_update_env(void)
{
	struct eth_device *eth_current = eth_get_current();
	char tmp[22];

	if (NetOurGatewayIP)
		dev_set_param_ip(&eth_current->dev, "gateway", NetOurGatewayIP);

	if (NetOurSubnetMask)
		dev_set_param_ip(&eth_current->dev, "netmask", NetOurSubnetMask);


	if (NetOurHostName[0])
		setenv ("hostname", NetOurHostName);

	if (NetOurRootPath[0])
		setenv ("rootpath", NetOurRootPath);

	if (NetOurIP)
		dev_set_param_ip(&eth_current->dev, "ipaddr", NetOurIP);

	if (NetServerIP)
		dev_set_param_ip(&eth_current->dev, "serverip", NetServerIP);

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

static int do_tftpb (cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	return netboot_common (TFTP, cmdtp, argc, argv);
}

static const __maybe_unused char cmd_tftp_help[] =
"Usage: tftp <file> [localfile]\n"
"Load a file via network using BootP/TFTP protocol.\n";

BAREBOX_CMD_START(tftp)
	.cmd		= do_tftpb,
	.usage		= "Load file using tftp protocol",
	BAREBOX_CMD_HELP(cmd_tftp_help)
BAREBOX_CMD_END

/**
 * @page tftp_command tftp
 *
 * Usage is: tftp \<filename\> [\<localfilename\>]
 *
 * Load a file via network using BootP/TFTP protocol. The loaded file you
 * can find after download in you current ramdisk. Refer \b ls command.
 *
 * \<localfile> can be the local filename only, or also a device name. In the
 * case of a device name, the will gets stored there. This works also for
 * partitions of flash memory. Refer \b erase, \b unprotect for flash
 * preparation.
 *
 * Note: This command is available only, if enabled in the menuconfig.
 */

#ifdef CONFIG_NET_RARP
extern void RarpRequest(void);

static int do_rarpb (cmd_tbl_t *cmdtp, int argc, char *argv[])
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

#ifdef CONFIG_NET_NFS
static int do_nfs (cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	return netboot_common(NFS, cmdtp, argc, argv);
}

static const __maybe_unused char cmd_nfs_help[] =
"Usage: nfs <file> [localfile]\n"
"Load a file via network using nfs protocol.\n";

BAREBOX_CMD_START(nfs)
	.cmd		= do_nfs,
	.usage		= "boot image via network using nfs protocol",
	BAREBOX_CMD_HELP(cmd_nfs_help)
BAREBOX_CMD_END

#endif	/* CONFIG_NET_NFS */

int net_store_fd;

extern void TftpStart(char *);       /* Begin TFTP get */
extern void NfsStart(char *);

static int
netboot_common (proto_t proto, cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int   rcode = 0;
	int   size;
	char  *localfile;
	char  *remotefile;

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	remotefile = argv[1];

	if (argc == 2)
		localfile = basename(remotefile);
	else
		localfile = argv[2];

	net_store_fd = open(localfile, O_WRONLY | O_CREAT);
	if (net_store_fd < 0) {
		perror("open");
		return 1;
	}

	if (NetLoopInit(proto) < 0)
		goto out;

	switch (proto) {
	case TFTP:
		TftpStart(remotefile);
		break;
	case NFS:
		NfsStart(remotefile);
	default:
		break;
	}

	if ((size = NetLoop()) < 0) {
		rcode = size;
		goto out;
	}

	/* NetLoop ok, update environment */
	netboot_update_env();

out:
	close(net_store_fd);
	return rcode;
}

#ifdef CONFIG_NET_CDP

static void cdp_update_env(void)
{
	char tmp[16];

	if (CDPApplianceVLAN != htons(-1)) {
		printf("CDP offered appliance VLAN %d\n", ntohs(CDPApplianceVLAN));
		VLAN_to_string(CDPApplianceVLAN, tmp);
		setenv("vlan", tmp);
		NetOurVLAN = CDPApplianceVLAN;
	}

	if (CDPNativeVLAN != htons(-1)) {
		printf("CDP offered native VLAN %d\n", ntohs(CDPNativeVLAN));
		VLAN_to_string(CDPNativeVLAN, tmp);
		setenv("nvlan", tmp);
		NetOurNativeVLAN = CDPNativeVLAN;
	}

}

static int do_cdp (cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int r;

	if (NetLoopInit(CDP) < 0)
		return 1;

	r = NetLoop();
	if (r < 0) {
		printf("cdp failed; perhaps not a CISCO switch?\n");
		return 1;
	}

	cdp_update_env();

	return 0;
}

BAREBOX_CMD_START(cdp)
	.cmd		= do_cdp,
	.usage		= "Perform CDP network configuration",
	BAREBOX_CMD_HELP("[loadAddress] [host ip addr:bootfilename]\n")
BAREBOX_CMD_END

#endif	/* CONFIG_NET_CDP */

static int do_ethact (cmd_tbl_t *cmdtp, int argc, char *argv[])
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

