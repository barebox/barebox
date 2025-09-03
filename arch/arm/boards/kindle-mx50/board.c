// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2007 Sascha Hauer, Pengutronix
// SPDX-FileCopyrightText: 2017 Alexander Kurz <akurz@blala.de>

#include <common.h>
#include <envfs.h>
#include <environment.h>
#include <init.h>
#include <io.h>
#include <driver.h>
#include <param.h>
#include <magicvar.h>
#include <libfile.h>
#include <globalvar.h>
#include <asm/armlinux.h>
#include <asm/mach-types.h>
#include <linux/sizes.h>
#include <linux/usb/fsl_usb2.h>
#include <mach/imx/generic.h>
#include <mach/imx/imx50-regs.h>
#include <mach/imx/imx5.h>
#include <mach/imx/revision.h>

/* 16 byte id for the device serial number */
#define ATAG_SERIAL16   0x5441000a
/* 16 byte id for a board revision */
#define ATAG_REVISION16 0x5441000b
/* mac address / secret */
#define ATAG_MACADDR    0x5441000d
/* 2x 16 byte /proc/bootmode */
#define ATAG_BOOTMODE   0x5441000f

struct tag_char16 {
	char data[16];
};

struct tag_macaddr {
	char address[12];
	char secret[20];
};

struct tag_bootmode {
	char boot[16];
	char post[16];
};

static struct tag *setup_16char_tag(struct tag *params, uint32_t tag,
				    const char *value)
{
	struct tag_char16 *target;

	target = (void *)params + sizeof(struct tag_header);
	params->hdr.tag = tag;
	params->hdr.size = tag_size(tag_char16);
	memset(target->data, 0, sizeof(target->data));
	/* don't care for missing null terminators */
	strncpy(target->data, value, sizeof(target->data));
	return tag_next(params);
}

static struct tag *setup_macaddr_tag(struct tag *params, uint32_t tag,
				    const char *addr, const char *secret)
{
	struct tag_macaddr *target;

	target = (void *)params + sizeof(struct tag_header);
	params->hdr.tag = tag;
	params->hdr.size = tag_size(tag_macaddr);
	memset(target->address, 0, sizeof(target->address));
	memset(target->secret, 0, sizeof(target->secret));
	strncpy(target->address, addr, sizeof(target->address));
	strncpy(target->secret, secret, sizeof(target->secret));
	return tag_next(params);
}

static struct tag *setup_bootmode_tag(struct tag *params, uint32_t tag,
				    const char *bootmode, const char *postmode)
{
	struct tag_bootmode *target;

	target = (void *) params + sizeof(struct tag_header);
	params->hdr.tag = tag;
	params->hdr.size = tag_size(tag_macaddr);
	memset(target->boot, 0, sizeof(target->boot));
	memset(target->post, 0, sizeof(target->post));
	strncpy(target->boot, bootmode, sizeof(target->boot));
	strncpy(target->post, postmode, sizeof(target->post));
	return tag_next(params);
}

static const char *get_env_char_tag(const char *tag, const char *defaultval)
{
	const char *value;
	value = getenv(tag);
	if (!value) {
		printf("env var %s not found, using default\n", tag);
		return defaultval;
	}
	return value;
}

BAREBOX_MAGICVAR(global.board.serial16,
	"The device serial in ATAG_SERIAL16, visible in /proc/cpuinfo");
BAREBOX_MAGICVAR(global.board.revision16,
	"The boardid in ATAG_REVISION16 to fill /proc/boardid");
BAREBOX_MAGICVAR(global.board.mac,
	"The wifi MAC in ATAG_MACADDR visible as /proc/mac_addr");
BAREBOX_MAGICVAR(global.board.sec,
	"The sec code in ATAG_MACADDR visible as /proc/mac_sec");
BAREBOX_MAGICVAR(global.board.bootmode,
	"The bootmode in ATAG_BOOTMODE to fill /proc/bootmode");
BAREBOX_MAGICVAR(global.board.postmode,
	"The postmode in ATAG_BOOTMODE to fill /proc/postmode");

static struct envdata {
	size_t offset;
	size_t size;
	const char *env_name;
	char *data;
} kindle_mx50_boardinfo[] = {
	{ 0x0,  16, "board.serial16" },
	{ 0x60, 16, "board.revision16" },
	{ 0x30, 16, "board.mac" },
	{ 0x40, 20, "board.sec" },
	{ 0x1000, 16, "board.bootmode" },
	{ 0x1010, 16, "board.postmode" },
};

/* The original kernel expects multiple custom ATAGs. */
static struct tag *kindle_mx50_append_atags(struct tag *params)
{
	params = setup_16char_tag(params, ATAG_SERIAL16,
			get_env_char_tag("global.board.serial16",
				"0000000000000000"));
	params = setup_16char_tag(params, ATAG_REVISION16,
			get_env_char_tag("global.board.revision16",
				"0000000000000000"));
	params = setup_macaddr_tag(params, ATAG_MACADDR,
			get_env_char_tag("global.board.mac",
				"000000000000"),
			get_env_char_tag("global.board.sec",
				"00000000000000000000"));
	params = setup_bootmode_tag(params, ATAG_BOOTMODE,
			get_env_char_tag("global.board.bootmode", ""),
			get_env_char_tag("global.board.postmode", ""));
	return params;
}

static void kindle_rev_init(const char *part, struct envdata *env, size_t num)
{
	void *buf;

	int ret = read_file_2(part, NULL, &buf, 0x1060);
	if (ret && ret != -EFBIG) {
		pr_err("Could not read board info from %s\n", part);
		return;
	}
	for (int i = 0; i < num; i++) {
		size_t offset = env[i].offset;
		size_t size = env[i].size;
		const char *name = env[i].env_name;

		env[i].data = xzalloc(size + 1);
		env[i].data[size] = 0;
		memcpy(env[i].data, buf + offset, size);
		globalvar_add_simple_string(name, &env[i].data);
	}

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
	kindle_rev_init("/dev/mmc2.boot0.userdata", kindle_mx50_boardinfo,
		ARRAY_SIZE(kindle_mx50_boardinfo));

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
	struct device *dev;

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
