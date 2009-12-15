/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * Copyright (C) 2001  Erik Mouw (J.A.K.Mouw@its.tudelft.nl)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 *
 */

#include <boot.h>
#include <common.h>
#include <command.h>
#include <driver.h>
#include <environment.h>
#include <image.h>
#include <zlib.h>
#include <init.h>
#include <fs.h>
#include <linux/list.h>
#include <xfuncs.h>
#include <malloc.h>
#include <fcntl.h>

#include <asm/byteorder.h>
#include <asm/global_data.h>
#include <asm/setup.h>
#include <asm/barebox-arm.h>

static struct tag *params;
static int armlinux_architecture = 0;
static void *armlinux_bootparams = NULL;

static void setup_start_tag(void)
{
	params = (struct tag *)armlinux_bootparams;

	params->hdr.tag = ATAG_CORE;
	params->hdr.size = tag_size(tag_core);

	params->u.core.flags = 0;
	params->u.core.pagesize = 0;
	params->u.core.rootdev = 0;

	params = tag_next(params);
}

struct arm_memory {
	struct list_head list;
	struct device_d *dev;
};

static LIST_HEAD(memory_list);

static void setup_memory_tags(void)
{
	struct arm_memory *mem;

	list_for_each_entry(mem, &memory_list, list) {
		params->hdr.tag = ATAG_MEM;
		params->hdr.size = tag_size(tag_mem32);

		params->u.mem.start = mem->dev->map_base;
		params->u.mem.size = mem->dev->size;

		params = tag_next(params);
	}
}

static void setup_commandline_tag(const char *commandline)
{
	const char *p;

	if (!commandline)
		return;

	/* eat leading white space */
	for (p = commandline; *p == ' '; p++) ;

	/*
	 * skip non-existent command lines so the kernel will still
	 * use its default command line.
	 */
	if (*p == '\0')
		return;

	params->hdr.tag = ATAG_CMDLINE;
	params->hdr.size =
	    (sizeof (struct tag_header) + strlen(p) + 1 + 4) >> 2;

	strcpy(params->u.cmdline.cmdline, p);

	params = tag_next(params);
}

#if 0
static void setup_initrd_tag(ulong initrd_start, ulong initrd_end)
{
	/* an ATAG_INITRD node tells the kernel where the compressed
	 * ramdisk can be found. ATAG_RDIMG is a better name, actually.
	 */
	params->hdr.tag = ATAG_INITRD2;
	params->hdr.size = tag_size(tag_initrd);

	params->u.initrd.start = initrd_start;
	params->u.initrd.size = initrd_end - initrd_start;

	params = tag_next(params);
}
#endif

static void setup_end_tag (void)
{
	params->hdr.tag = ATAG_NONE;
	params->hdr.size = 0;
}

void armlinux_set_bootparams(void *params)
{
	armlinux_bootparams = params;
}

void armlinux_set_architecture(int architecture)
{
	armlinux_architecture = architecture;
}

void armlinux_add_dram(struct device_d *dev)
{
	struct arm_memory *mem = xzalloc(sizeof(*mem));

	mem->dev = dev;

	list_add_tail(&mem->list, &memory_list);
}

#ifdef CONFIG_CMD_BOOTM
int do_bootm_linux(struct image_data *data)
{
	void (*theKernel)(int zero, int arch, void *params);
	image_header_t *os_header = &data->os->header;
	const char *commandline = getenv("bootargs");

	if (os_header->ih_type == IH_TYPE_MULTI) {
		printf("Multifile images not handled at the moment\n");
		return -1;
	}

	if (armlinux_architecture == 0) {
		printf("arm architecture not set. Please specify with -a option\n");
		return -1;
	}

	if (!armlinux_bootparams) {
		printf("Bootparams not set. Please fix your board code\n");
		return -1;
	}

	printf("commandline: %s\n"
	       "arch_number: %d\n", commandline, armlinux_architecture);

	theKernel = (void *)ntohl(os_header->ih_ep);

	debug("## Transferring control to Linux (at address 0x%p) ...\n",
	       theKernel);

	setup_start_tag();
	setup_memory_tags();
	setup_commandline_tag(commandline);
#if 0
	if (initrd_start && initrd_end)
		setup_initrd_tag (initrd_start, initrd_end);
#endif
	setup_end_tag();

	if (relocate_image(data->os, (void *)ntohl(os_header->ih_load)))
		return -1;

	/* we assume that the kernel is in place */
	printf("\nStarting kernel ...\n\n");

	cleanup_before_linux();
	theKernel (0, armlinux_architecture, armlinux_bootparams);

	return -1;
}

static int image_handle_cmdline_parse(struct image_data *data, int opt,
		char *optarg)
{
	switch (opt) {
	case 'a':
		armlinux_architecture = simple_strtoul(optarg, NULL, 0);
		return 0;
	default:
		return 1;
	}
}

static struct image_handler handler = {
	.cmdline_options = "a:",
	.cmdline_parse = image_handle_cmdline_parse,
	.help_string = " -a <arch>      use architecture number <arch>",

	.bootm = do_bootm_linux,
	.image_type = IH_OS_LINUX,
};

static int armlinux_register_image_handler(void)
{
	return register_image_handler(&handler);
}

late_initcall(armlinux_register_image_handler);
#endif /* CONFIG_CMD_BOOTM */

#ifdef CONFIG_CMD_BOOTZ
struct zimage_header {
	u32	unsused[9];
	u32	magic;
	u32	start;
	u32	end;
};

static int do_bootz(cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	void (*theKernel)(int zero, int arch, void *params);
	const char *commandline = getenv("bootargs");
	int fd, ret;
	struct zimage_header header;
	void *zimage;

	if (argc != 2) {
		barebox_cmd_usage(cmdtp);
		return 1;
	}

	fd = open(argv[1], O_RDONLY);

	ret = read(fd, &header, sizeof(header));
	if (ret < sizeof(header)) {
		printf("could not read %s\n", argv[1]);
		goto err_out;
	}

	if (header.magic != 0x016f2818) {
		printf("invalid magic 0x%08x\n", header.magic);
		goto err_out;
	}

	zimage = xmalloc(header.end);
	memcpy(zimage, &header, sizeof(header));

	ret = read(fd, zimage + sizeof(header), header.end - sizeof(header));
	if (ret < header.end - sizeof(header)) {
		printf("could not read %s\n", argv[1]);
		goto err_out1;
	}

	theKernel = zimage;

	printf("loaded zImage from %s with size %d\n", argv[1], header.end);

	setup_start_tag();
	setup_memory_tags();
	setup_commandline_tag(commandline);
#if 0
	if (initrd_start && initrd_end)
		setup_initrd_tag (initrd_start, initrd_end);
#endif
	setup_end_tag();

	cleanup_before_linux();
	theKernel(0, armlinux_architecture, armlinux_bootparams);

	return 0;

err_out1:
	free(zimage);
err_out:
	close(fd);

	return 1;
}

static const __maybe_unused char cmd_bootz_help[] =
"Usage: bootz [FILE]\n"
"Boot a Linux zImage\n";

BAREBOX_CMD_START(bootz)
	.cmd            = do_bootz,
	.usage          = "bootz - start a zImage",
	BAREBOX_CMD_HELP(cmd_bootz_help)
BAREBOX_CMD_END
#endif /* CONFIG_CMD_BOOTZ */

#ifdef CONFIG_CMD_BOOTU
static int do_bootu(cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	void (*theKernel)(int zero, int arch, void *params);
	const char *commandline = getenv("bootargs");

	if (argc != 2) {
		barebox_cmd_usage(cmdtp);
		return 1;
	}

	theKernel = (void *)simple_strtoul(argv[1], NULL, 0);

	setup_start_tag();
	setup_memory_tags();
	setup_commandline_tag(commandline);
	setup_end_tag();

	cleanup_before_linux();
	theKernel(0, armlinux_architecture, armlinux_bootparams);

	return 1;
}

static const __maybe_unused char cmd_bootu_help[] =
"Usage: bootu <address>\n";

BAREBOX_CMD_START(bootu)
	.cmd            = do_bootu,
	.usage          = "bootu - start a raw linux image",
	BAREBOX_CMD_HELP(cmd_bootu_help)
BAREBOX_CMD_END
#endif /* CONFIG_CMD_BOOTU */
