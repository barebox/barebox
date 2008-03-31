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

static void netboot_update_env (void)
{
	struct eth_device *eth_current = eth_get_current();
	char tmp[22];

	if (NetOurGatewayIP)
		dev_set_param_ip(eth_current->dev, "gateway", NetOurGatewayIP);

	if (NetOurSubnetMask)
		dev_set_param_ip(eth_current->dev, "netmask", NetOurSubnetMask);


	if (NetOurHostName[0])
		setenv ("hostname", NetOurHostName);

	if (NetOurRootPath[0])
		setenv ("rootpath", NetOurRootPath);

	if (NetOurIP)
		dev_set_param_ip(eth_current->dev, "ipaddr", NetOurIP);

	if (NetServerIP)
		dev_set_param_ip(eth_current->dev, "serverip", NetServerIP);

	if (NetOurDNSIP) {
		ip_to_string (NetOurDNSIP, tmp);
		setenv ("dnsip", tmp);
	}
#if (CONFIG_BOOTP_MASK & CONFIG_BOOTP_DNS2)
	if (NetOurDNS2IP) {
		ip_to_string (NetOurDNS2IP, tmp);
		setenv ("dnsip2", tmp);
	}
#endif
	if (NetOurNISDomain[0])
		setenv ("domain", NetOurNISDomain);

#if defined CONFIG_NET_SNTP && (CONFIG_BOOTP_MASK & CONFIG_BOOTP_TIMEOFFSET)
	if (NetTimeOffset) {
		sprintf (tmp, "%d", NetTimeOffset);
		setenv ("timeoffset", tmp);
	}
#endif
#if defined CONFIG_NET_SNTP && (CONFIG_BOOTP_MASK & CONFIG_BOOTP_NTPSERVER)
	if (NetNtpServerIP) {
		ip_to_string (NetNtpServerIP, tmp);
		setenv ("ntpserverip", tmp);
	}
#endif
}

static int do_tftpb (cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	return netboot_common (TFTP, cmdtp, argc, argv);
}

static __maybe_unused char cmd_tftp_help[] =
"Usage: tftp <file> [localfile]\n"
"Load a file via network using BootP/TFTP protocol.\n";

U_BOOT_CMD_START(tftp)
	.maxargs	= 3,
	.cmd		= do_tftpb,
	.usage		= "Load file using tftp protocol",
	U_BOOT_CMD_HELP(cmd_tftp_help)
U_BOOT_CMD_END

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
static int do_rarpb (cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	return netboot_common (RARP, cmdtp, argc, argv);
}

U_BOOT_CMD_START(rarpboot)
	.maxargs	= 3,
	.cmd		= do_rarpb,
	.usage		= "boot image via network using rarp/tftp protocol",
	U_BOOT_CMD_HELP("[loadAddress] [bootfilename]\n")
U_BOOT_CMD_END
#endif /* CONFIG_NET_RARP */

/**
 * @page rarp_command rarp
 *
 * Usage is: FIXME
 *
 * Load a file via network using rarp/tftp protocol. FIXME: Where to find it
 * after loading?
 * @note This command is available only, if enabled in the menuconfig.
 */

#ifdef CONFIG_NET_DHCP
static int do_dhcp (cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int size;

	if ((size = NetLoop(DHCP)) < 0)
		return 1;

	/* NetLoop ok, update environment */
	netboot_update_env();

	return 0;
}

U_BOOT_CMD_START(dhcp)
	.maxargs	= 1,
	.cmd		= do_dhcp,
	.usage		= "invoke dhcp client to obtain ip/boot params",
U_BOOT_CMD_END

#endif	/* CONFIG_NET_DHCP */

#ifdef CONFIG_NET_NFS
static int do_nfs (cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	return netboot_common(NFS, cmdtp, argc, argv);
}

static __maybe_unused char cmd_nfs_help[] =
"Usage: nfs <file> [localfile]\n"
"Load a file via network using nfs protocol.\n";

U_BOOT_CMD_START(nfs)
	.maxargs	= 3,
	.cmd		= do_nfs,
	.usage		= "boot image via network using nfs protocol",
	U_BOOT_CMD_HELP(cmd_nfs_help)
U_BOOT_CMD_END

#endif	/* CONFIG_NET_NFS */

int net_store_fd;

static int
netboot_common (proto_t proto, cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int   rcode = 0;
	int   size;
	char  *localfile;
	char  *remotefile;

	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

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

	safe_strncpy (BootFile, remotefile, sizeof(BootFile));

	if ((size = NetLoop(proto)) < 0) {
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

	r = NetLoop(CDP);
	if (r < 0) {
		printf("cdp failed; perhaps not a CISCO switch?\n");
		return 1;
	}

	cdp_update_env();

	return 0;
}

U_BOOT_CMD_START(cdp)
	.maxargs	= 1,
	.cmd		= do_cdp,
	.usage		= "Perform CDP network configuration",
	U_BOOT_CMD_HELP("[loadAddress] [host ip addr:bootfilename]\n")
U_BOOT_CMD_END

#endif	/* CONFIG_NET_CDP */
