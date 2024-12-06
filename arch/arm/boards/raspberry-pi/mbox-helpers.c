// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2009 Carlo Caione <carlo@carlocaione.org>

#include <mach/bcm283x/mbox.h>
#include <linux/printk.h>
#include "lowlevel.h"

struct msg_get_arm_mem {
	struct bcm2835_mbox_hdr hdr;
	struct bcm2835_mbox_tag_get_arm_mem get_arm_mem;
	u32 end_tag;
};

struct msg_get_board_rev {
	struct bcm2835_mbox_hdr hdr;
	struct bcm2835_mbox_tag_get_board_rev get_board_rev;
	u32 end_tag;
};

struct msg_get_mac_address {
	struct bcm2835_mbox_hdr hdr;
	struct bcm2835_mbox_tag_get_mac_address get_mac_address;
	u32 end_tag;
};

ssize_t rpi_get_arm_mem(void)
{
	BCM2835_MBOX_STACK_ALIGN(struct msg_get_arm_mem, msg);
	int ret;

	BCM2835_MBOX_INIT_HDR(msg);
	BCM2835_MBOX_INIT_TAG(&msg->get_arm_mem, GET_ARM_MEMORY);

	ret = bcm2835_mbox_call_prop(BCM2835_MBOX_PROP_CHAN, &msg->hdr);
	if (ret)
		return ret;

	return msg->get_arm_mem.body.resp.mem_size;
}

int rpi_get_usbethaddr(u8 mac[6])
{
	BCM2835_MBOX_STACK_ALIGN(struct msg_get_mac_address, msg);
	int ret;

	BCM2835_MBOX_INIT_HDR(msg);
	BCM2835_MBOX_INIT_TAG(&msg->get_mac_address, GET_MAC_ADDRESS);

	ret = bcm2835_mbox_call_prop(BCM2835_MBOX_PROP_CHAN, &msg->hdr);
	if (ret) {
		pr_info("bcm2835: Could not query MAC address\n");
		return ret;
	}

	memcpy(mac, msg->get_mac_address.body.resp.mac, 6);
	return 0;
}

int rpi_get_board_rev(void)
{
	int ret;

	BCM2835_MBOX_STACK_ALIGN(struct msg_get_board_rev, msg);
	BCM2835_MBOX_INIT_HDR(msg);
	BCM2835_MBOX_INIT_TAG(&msg->get_board_rev, GET_BOARD_REV);

	ret = bcm2835_mbox_call_prop(BCM2835_MBOX_PROP_CHAN, &msg->hdr);
	if (ret) {
		pr_err("Could not query board revision\n");
		return ret;
	}

	return msg->get_board_rev.body.resp.rev;
}
