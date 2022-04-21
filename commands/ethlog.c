// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: (c) 2022 Pengutronix,
//                             Oleksij Rempel <o.rempel@pengutronix.de>

#include <common.h>
#include <command.h>
#include <complete.h>
#include <environment.h>
#include <getopt.h>
#include <net.h>

static void ethlog_rx_monitor(struct eth_device *edev, void *packet,
			       int length)
{
	dev_print_hex_dump(&edev->dev, KERN_DEBUG, "rx data <: ",
			   DUMP_PREFIX_OFFSET, 16, 1, packet, length, true);
	printk("\n");
}

static void ethlog_tx_monitor(struct eth_device *edev, void *packet,
			       int length)
{
	dev_print_hex_dump(&edev->dev, KERN_DEBUG, "tx data >: ",
			   DUMP_PREFIX_OFFSET, 16, 1, packet, length, true);
	printk("\n");
}

static int do_ethlog(int argc, char *argv[])
{
	struct eth_device *edev;
	const char *edevname;
	bool remove = false;
	int opt;

	while ((opt = getopt(argc, argv, "r")) > 0) {
		switch (opt) {
		case 'r':
			remove = true;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (optind == argc)
		edevname = "eth0";
	else
		edevname = argv[optind];

	edev = eth_get_byname(edevname);
	if (!edev) {
		printf("No such network device: %s\n", edevname);
		return 1;
	}

	if (remove) {
		edev->tx_monitor = NULL;
		edev->rx_monitor = NULL;

		return 0;
	}

	edev->tx_monitor = ethlog_tx_monitor;
	edev->rx_monitor = ethlog_rx_monitor;

	return 0;
}

BAREBOX_CMD_HELP_START(ethlog)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-r", "remove log handler from Ethernet interface")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(ethlog)
	.cmd		= do_ethlog,
	BAREBOX_CMD_DESC("ETHLOG - tool to get dump of Ethernet packets")
	BAREBOX_CMD_OPTS("[-r] [device]")
	BAREBOX_CMD_GROUP(CMD_GRP_NET)
	BAREBOX_CMD_COMPLETE(eth_complete)
BAREBOX_CMD_END
