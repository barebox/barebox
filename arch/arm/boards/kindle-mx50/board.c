/*
 * Copyright (C) 2007 Sascha Hauer, Pengutronix
 * Copyright (C) 2017 Alexander Kurz <akurz@blala.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <envfs.h>
#include <environment.h>
#include <init.h>
#include <io.h>
#include <driver.h>
#include <param.h>
#include <magicvar.h>
#include <partition.h>
#include <libfile.h>
#include <globalvar.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <linux/sizes.h>
#include <usb/fsl_usb2.h>
#include <mach/generic.h>
#include <mach/imx50-regs.h>
#include <mach/imx5.h>
#include <mach/revision.h>

/* 16 byte id for serial number */
#define ATAG_SERIAL16   0x5441000a
/* 16 byte id for a board revision */
#define ATAG_REVISION16 0x5441000b

struct char16_tag {
	char data[16];
};

static struct tag *setup_16char_tag(struct tag *params, uint32_t tag,
				    const char *value)
{
	struct char16_tag *target;
	target = ((void *) params) + sizeof(struct tag_header);
	params->hdr.tag = tag;
	params->hdr.size = tag_size(char16_tag);
	memcpy(target->data, value, sizeof target->data);
	return tag_next(params);
}

static const char *get_env_16char_tag(const char *tag)
{
	static const char *default16 = "0000000000000000";
	const char *value;
	value = getenv(tag);
	if (!value) {
		pr_err("env var %s not found, using default\n", tag);
		return default16;
	}
	if (strlen(value) != 16) {
		pr_err("env var %s: expecting 16 characters, using default\n",
			tag);
		return default16;
	}
	pr_info("%s: %s\n", tag, value);
	return value;
}

BAREBOX_MAGICVAR_NAMED(global_atags_serial16, global.board.serial16,
	"Pass the kindle Serial as vendor-specific ATAG to linux");
BAREBOX_MAGICVAR_NAMED(global_atags_revision16, global.board.revision16,
	"Pass the kindle BoardId as vendor-specific ATAG to linux");

/* The Kindle Kernel expects two custom ATAGs, ATAG_REVISION16 describing
 * the board and ATAG_SERIAL16 to identify the individual device.
 */
static struct tag *kindle_mx50_append_atags(struct tag *params)
{
	params = setup_16char_tag(params, ATAG_SERIAL16,
				get_env_16char_tag("global.board.serial16"));
	params = setup_16char_tag(params, ATAG_REVISION16,
				get_env_16char_tag("global.board.revision16"));
	return params;
}

static char *serial16;
static char *revision16;
static char *mac;

static void kindle_rev_init(void)
{
	int ret;
	size_t size;
	void *buf;
	const char userdata[] = "/dev/disk0.boot0.userdata";
	ret = read_file_2(userdata, &size, &buf, 128);
	if (ret && ret != -EFBIG) {
		pr_err("Could not read board info from %s\n", userdata);
		return;
	}

	serial16 = xzalloc(17);
	revision16 = xzalloc(17);
	mac = xzalloc(17);

	memcpy(serial16, buf, 16);
	memcpy(revision16, buf + 96, 16);
	memcpy(mac, buf + 48, 16);

	globalvar_add_simple_string("board.serial16", &serial16);
	globalvar_add_simple_string("board.revision16", &revision16);
	globalvar_add_simple_string("board.mac", &mac);

	free(buf);
}

static int is_mx50_kindle(void)
{
	return of_machine_is_compatible("amazon,kindle-d01100") ||
		of_machine_is_compatible("amazon,kindle-d01200") ||
		of_machine_is_compatible("amazon,kindle-ey21");
}

static int kindle_mx50_late_init(void)
{
	if (!is_mx50_kindle())
		return 0;

	armlinux_set_revision(0x50000 | imx_silicon_revision());
	/* Compatibility ATAGs for original kernel */
	armlinux_set_atag_appender(kindle_mx50_append_atags);

	kindle_rev_init();

	return 0;
}
late_initcall(kindle_mx50_late_init);

static int kindle_mx50_mem_init(void)
{
	if (!is_mx50_kindle())
		return 0;

	arm_add_mem_device("ram0", MX50_CSD0_BASE_ADDR, SZ_256M);
	return 0;
}
mem_initcall(kindle_mx50_mem_init);

static int kindle_mx50_devices_init(void)
{
	struct device_d *dev;

	if (!is_mx50_kindle())
		return 0;

	/* Probe the eMMC to allow reading the board serial and revision */
	dev = get_device_by_name("mci0");
	if (dev)
		dev_set_param(dev, "probe", "1");

	defaultenv_append_directory(defaultenv_kindle_mx50);

	return 0;
}
device_initcall(kindle_mx50_devices_init);

static int kindle_mx50_postcore_init(void)
{
	if (!is_mx50_kindle())
		return 0;

	imx50_init_lowlevel(800);

	return 0;
}
postcore_initcall(kindle_mx50_postcore_init);
