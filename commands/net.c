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

