/*
 * usb.c - rescan for USB devices
 *
 * Copyright (c) 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <common.h>
#include <command.h>
#include <complete.h>
#include <usb/usb.h>
#include <getopt.h>

/* shows the device tree recursively */
static void usb_show_tree_graph(struct usb_device *dev, char *pre)
{
	int i, index;
	int has_child, last_child;

	index = strlen(pre);
	printf(" %s", pre);
	/* check if the device has connected children */
	has_child = 0;

	for (i = 0; i < dev->maxchild; i++) {
		if (dev->children[i] != NULL)
			has_child = 1;
	}

	/* check if we are the last one */
	last_child = 1;

	if (dev->parent) {
		for (i = 0; i < dev->parent->maxchild; i++) {
			if (dev->parent->children[i])
				last_child = 0;

			if (dev->parent->children[i] == dev)
				last_child = 1;
		} /* for all children of the parent */
		printf("\b+-");

		/* correct last child */
		if (last_child)
			pre[index - 1] = ' ';
	} else {
		/* if not root hub */
		printf(" ");
	}

	printf("%d ", dev->devnum);

	pre[index++] = ' ';
	pre[index++] = has_child ? '|' : ' ';
	pre[index] = 0;

	printf("ID %04x:%04x\n", dev->descriptor->idVendor, dev->descriptor->idProduct);

	if (strlen(dev->mf) || strlen(dev->prod) || strlen(dev->serial))
		printf(" %s  %s %s %s\n", pre, dev->mf, dev->prod, dev->serial);

	printf(" %s\n", pre);

	if (dev->maxchild > 0) {
		for (i = 0; i < dev->maxchild; i++) {
			if (dev->children[i] != NULL) {
				usb_show_tree_graph(dev->children[i], pre);
				pre[index] = 0;
			}
		}
	}
}

/* main routine for the tree command */
static void usb_show_tree(struct usb_device *dev)
{
	char preamble[32];

	memset(preamble, 0, 32);
	usb_show_tree_graph(dev, &preamble[0]);
}

static void usb_show_devices(bool tree)
{
	struct usb_device *dev;

	list_for_each_entry(dev, &usb_device_list, list) {
		if (tree) {
			if (dev->parent == NULL)
				usb_show_tree(dev);
		} else {
			printf("Bus %03d Device %03d: ID %04x:%04x %s\n",
				dev->host->busnum, dev->devnum,
				dev->descriptor->idVendor,
				dev->descriptor->idProduct,
				dev->prod);
		}
	}
}

static int do_usb(int argc, char *argv[])
{
	int opt;
	int tree = 0, show = 0;

	while ((opt = getopt(argc, argv, "ts")) > 0) {
		switch (opt) {
		case 't':
			tree = 1;
			show = 1;
			break;
		case 's':
			show = 1;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	usb_rescan();

	if (show)
		usb_show_devices(tree);

	return 0;
}

BAREBOX_CMD_HELP_START(usb)
BAREBOX_CMD_HELP_TEXT("Scan for USB devices.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-s", "show devices")
BAREBOX_CMD_HELP_OPT("-t", "show USB tree")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(usb)
	.cmd		= do_usb,
	BAREBOX_CMD_DESC("(re-)detect USB devices")
	BAREBOX_CMD_OPTS("[-ts]")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_usb_help)
	BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END
