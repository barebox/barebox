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

/*
 * Boot support
 */
#include <common.h>
#include <command.h>
#include <environment.h>
#include <driver.h>
#include <net.h>
#include <fs.h>
#include <fcntl.h>
#include <errno.h>

extern int do_bootm (cmd_tbl_t *, int, int, char *[]);

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
		dev_set_param_ip(eth_current->dev, "ip", NetOurIP);

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

int do_bootp (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	return netboot_common (BOOTP, cmdtp, argc, argv);
}

U_BOOT_CMD_START(bootp)
	.maxargs	= 3,
	.cmd		= do_bootp,
	.usage		= "boot image via network using bootp/tftp protocol",
	U_BOOT_CMD_HELP("[loadAddress] [bootfilename]\n")
U_BOOT_CMD_END

int do_tftpb (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	return netboot_common (TFTP, cmdtp, argc, argv);
}

static __maybe_unused char cmd_tftpboot_help[] =
"Usage: tftpboot <localfile> <remotefile>\n"
"Load a file via network using BootP/TFTP protocol\n";

U_BOOT_CMD_START(tftpboot)
	.maxargs	= 3,
	.cmd		= do_tftpb,
	.usage		= "boot image via network using tftp protocol",
	U_BOOT_CMD_HELP(cmd_tftpboot_help)
U_BOOT_CMD_END

int do_rarpb (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	return netboot_common (RARP, cmdtp, argc, argv);
}

U_BOOT_CMD_START(rarpboot)
	.maxargs	= 3,
	.cmd		= do_rarpb,
	.usage		= "boot image via network using rarp/tftp protocol",
	U_BOOT_CMD_HELP("[loadAddress] [bootfilename]\n")
U_BOOT_CMD_END

#ifdef CONFIG_NET_DHCP
int do_dhcp (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int size;

	if ((size = NetLoop(DHCP)) < 0)
		return 1;

	/* NetLoop ok, update environment */
	netboot_update_env();

	return 0;
}

U_BOOT_CMD_START(dhcp)
	.maxargs	= 3,
	.cmd		= do_dhcp,
	.usage		= "invoke dhcp client to obtain ip/boot params",
U_BOOT_CMD_END

#endif	/* CONFIG_NET_DHCP */

#ifdef CONFIG_NET_NFS
int do_nfs (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	return netboot_common(NFS, cmdtp, argc, argv);
}

U_BOOT_CMD_START(nfs)
	.maxargs	= 3,
	.cmd		= do_nfs,
	.usage		= "boot image via network using nfs protocol",
	U_BOOT_CMD_HELP("[loadAddress] [host ip addr:bootfilename]\n")
U_BOOT_CMD_END

#endif	/* CONFIG_NET_NFS */

int net_store_fd;

static int
netboot_common (proto_t proto, cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int   rcode = 0;
	int   size;

	if (argc < 3) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	net_store_fd = open(argv[1], O_WRONLY | O_CREAT);
	if (net_store_fd < 0) {
		perror("open");
		return 1;
	}

	copy_filename (BootFile, argv[2], sizeof(BootFile));

	if ((size = NetLoop(proto)) < 0)
		return 1;

	/* NetLoop ok, update environment */
	netboot_update_env();

	/* done if no file was loaded (no errors though) */
	if (size == 0)
		return 0;

	/* flush cache */
	flush_cache(load_addr, size);

	return rcode;
}

#if (CONFIG_COMMANDS & CFG_CMD_CDP)

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

int do_cdp (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
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

#endif	/* CFG_CMD_CDP */
