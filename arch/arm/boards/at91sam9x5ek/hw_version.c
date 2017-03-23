/*
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
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
 *
 */

#include <common.h>
#include <init.h>
#include <fs.h>
#include <fcntl.h>
#include <libbb.h>
#include <libfile.h>
#include <asm/armlinux.h>
#include <of.h>

#include "hw_version.h"

enum board_type {
	BOARD_TYPE_EK,
	BOARD_TYPE_DM,
	BOARD_TYPE_CPU,
};

static struct board_info {
	char *name;
	enum board_type type;
	unsigned char id;
} board_list[] = {
	{"SAM9x5-EK",		BOARD_TYPE_EK,		0},
	{"SAM9x5-DM",		BOARD_TYPE_DM,		1},
	{"SAM9G15-CM",		BOARD_TYPE_CPU,		2},
	{"SAM9G25-CM",		BOARD_TYPE_CPU,		3},
	{"SAM9G35-CM",		BOARD_TYPE_CPU,		4},
	{"SAM9X25-CM",		BOARD_TYPE_CPU,		5},
	{"SAM9X35-CM",		BOARD_TYPE_CPU,		6},
	{"PDA-DM",		BOARD_TYPE_DM,		7},
};

static struct board_info* get_board_info_by_name(const char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(board_list); i++)
		if (strcmp(name, board_list[i].name) == 0)
			return &board_list[i];

	return NULL;
}

static struct vendor_info {
	char *name;
	enum vendor_id id;
} vendor_list[] = {
	{"EMBEST",		VENDOR_EMBEST},
	{"FLEX",		VENDOR_FLEX},
	{"RONETIX",		VENDOR_RONETIX},
	{"COGENT",		VENDOR_COGENT},
	{"PDA",			VENDOR_PDA},
};

static struct vendor_info* get_vendor_info_by_name(const char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(vendor_list); i++)
		if (strcmp(name, vendor_list[i].name) == 0)
			return &vendor_list[i];

	return NULL;
}

#define BOARD_NAME_LEN			12
#define VENDOR_NAME_LEN			10
#define VENDOR_COUNTRY_LEN		2

struct one_wire_info {
	u8 total_bytes;
	u8 vendor_name[VENDOR_NAME_LEN];
	u8 vendor_country[VENDOR_COUNTRY_LEN];
	u8 board_name[BOARD_NAME_LEN];
	u8 year;
	u8 week;
	u8 revision_code;
	u8 revision_id;
	u8 reserved;
	u8 checksum_l;
	u8 checksum_h;
}__attribute__ ((packed));

static int at91sam9x5ek_read_w1(const char *file, struct one_wire_info *info)
{
	int fd;
	int ret;

	fd = open(file, O_RDONLY);
	if (fd < 0) {
		ret = fd;
		goto err;
	}

	ret = read_full(fd, info, sizeof(*info));
	if (ret < 0)
		goto err_open;

	if (ret < sizeof(*info)) {
		ret =  -EINVAL;
		goto err_open;
	}

	pr_debug("total_bytes = %d\n", info->total_bytes);
	pr_debug("vendor_name = %s\n", info->vendor_name);
	pr_debug("vendor_country = %.2s\n", info->vendor_country);
	pr_debug("board_name = %s\n", info->board_name);
	pr_debug("year = %d\n", info->year);
	pr_debug("week = %d\n", info->week);
	pr_debug("revision_code = %x\n", info->revision_code);
	pr_debug("revision_id = %x\n", info->revision_id);
	pr_debug("reserved = %x\n", info->reserved);
	pr_debug("checksum_l = %x\n", info->checksum_l);
	pr_debug("checksum_h = %x\n", info->checksum_h);

	ret = 0;

err_open:
	close(fd);
err:
	if (ret)
		pr_err("can not read 1-wire %s (%s)\n", file, strerror(ret));
	return ret;
}

static u32 sn = 0;
static u32 rev = 0;

bool at91sam9x5ek_cm_is_vendor(enum vendor_id vid)
{
	return ((sn >> 5) & 0x1f) == vid;
}

bool at91sam9x5ek_ek_is_vendor(enum vendor_id vid)
{
	return ((sn >> 15) & 0x1f) == vid;
}

bool at91sam9x5ek_dm_is_vendor(enum vendor_id vid)
{
	return ((sn >> 25) & 0x1f) == vid;
}

static void at91sam9x5ek_devices_detect_one(const char *name)
{
	struct one_wire_info info;
	struct board_info* binfo;
	struct vendor_info* vinfo;
	struct device_d *dev = NULL;
	char str[16];
	u8 vendor_id = 0;

	if (at91sam9x5ek_read_w1(name, &info))
		return;

	binfo = get_board_info_by_name(info.board_name);

	if (!binfo) {
		pr_err("board %s no supported\n", info.board_name);
		return;
	}

	vinfo = get_vendor_info_by_name(info.vendor_name);
	if (vinfo)
		vendor_id = vinfo->id;

	switch (binfo->type) {
	case BOARD_TYPE_CPU:
		dev = add_generic_device_res("at91sam9x5cm", DEVICE_ID_SINGLE, NULL, 0, NULL);
		if (!dev)
			return;
		sn  |= (binfo->id & 0x1f);
		sn  |= ((vendor_id & 0x1f) << 5);
		rev |= (info.revision_code - 'A');
		rev |= (((info.revision_id - '0') & 0x3) << 15);
		pr_info("CM");
		break;
	case BOARD_TYPE_EK:
		dev = add_generic_device_res("at91sam9x5ek", DEVICE_ID_SINGLE, NULL, 0, NULL);
		if (!dev)
			return;
		sn  |= ((binfo->id & 0x1f) << 20);
		sn  |= ((vendor_id & 0x1f) << 25);
		rev |= ((info.revision_code - 'A') << 10);
		rev |= (((info.revision_id - '0') & 0x3) << 21);
		pr_info("EK");
		break;
	case BOARD_TYPE_DM:
		dev = add_generic_device_res("at91sam9x5dm", DEVICE_ID_SINGLE, NULL, 0, NULL);
		if (!dev)
			return;
		sn  |= ((binfo->id & 0x1f) << 10);
		sn  |= ((vendor_id & 0x1f) << 15);
		rev |= ((info.revision_code - 'A') << 5);
		rev |= (((info.revision_id - '0') & 0x3) << 18);
		pr_info("DM");
		break;
	}

	pr_info(": %s [%c%c] from %s\n",
		info.board_name, info.revision_code, info.revision_id,
		info.vendor_name);

	dev_add_param_fixed(dev, "vendor", info.vendor_name);
	dev_add_param_fixed(dev, "board", info.board_name);
	sprintf(str, "%.2s", info.vendor_country);
	dev_add_param_fixed(dev, "country", str);
	dev_add_param_int_ro(dev, "year", info.year, "%d");
	dev_add_param_int_ro(dev, "week", info.week, "%d");
	sprintf(str, "%c", info.revision_code);
	dev_add_param_fixed(dev, "revision_code", str);
	sprintf(str, "%c", info.revision_id);
	dev_add_param_fixed(dev, "revision_id", str);
}

#define NODE_NAME_LEN  128

static int cm_cogent_fixup(struct device_node *root, void *unused)
{
	int ret;
	struct device_node *node;

	for_each_compatible_node_from(node, root, NULL, "atmel,hsmci") {
		struct device_node *slotnode =
			of_get_child_by_name(node, "slot");
		if (!slotnode)
			continue;

		ret = of_set_property(slotnode, "broken-cd", NULL, 0, 1);
		if (ret)
			pr_err("error %d while adding broken-cd property to node %s\n",
				ret, slotnode->name);
	}

	return 0;
}

static int at91sam9x5ek_devices_detect_hw(void)
{
	at91sam9x5ek_devices_detect_one("/dev/ds24310");
	at91sam9x5ek_devices_detect_one("/dev/ds24311");
	at91sam9x5ek_devices_detect_one("/dev/ds24330");

	pr_info("sn: 0x%x, rev: 0x%x\n", sn, rev);
	armlinux_set_revision(rev);
	armlinux_set_serial(sn);

	if (at91sam9x5ek_cm_is_vendor(VENDOR_COGENT))
		of_register_fixup(cm_cogent_fixup, NULL);

	return 0;
}
late_initcall(at91sam9x5ek_devices_detect_hw);
