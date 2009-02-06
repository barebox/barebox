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

#include <asm/byteorder.h>
#include <asm/global_data.h>
#include <asm/setup.h>
#include <asm/u-boot-arm.h>

static struct tag *params;

#if defined (CONFIG_SETUP_MEMORY_TAGS) || \
    defined (CONFIG_CMDLINE_TAG) || \
    defined (CONFIG_INITRD_TAG) || \
    defined (CONFIG_SERIAL_TAG) || \
    defined (CONFIG_REVISION_TAG) || \
    defined (CONFIG_LCD) || \
    defined (CONFIG_VFD)
static void __setup_start_tag(void);
static void __setup_end_tag(void);
#define setup_start_tag __setup_start_tag
#define setup_end_tag __setup_end_tag
#else
#define setup_start_tag() do {} while(0)
#define setup_end_tag() do {} while(0)
#endif

#ifdef CONFIG_CMDLINE_TAG
static void __setup_commandline_tag(const char *commandline);
#define setup_commandline_tag __setup_commandline_tag
#else
#define setup_commandline_tag(a) do {} while(0)
#endif

#ifdef CONFIG_SETUP_MEMORY_TAGS
static void __setup_memory_tags(void);
#define setup_memory_tags __setup_memory_tags
#else
#define setup_memory_tags() do {} while(0)
#endif

#if 0
#ifdef CONFIG_INITRD_TAG
static void __setup_initrd_tag(ulong initrd_start, ulong initrd_end);
#define setup_initrd_tag __setup_initrd_tag
#else
#define setup_initrd_tag(a,b) do {} while(0)
#endif
#endif

extern ulong calc_fbsize(void);
#if defined (CONFIG_VFD) || defined (CONFIG_LCD)
static void __setup_videolfb_tag(gd_t *gd);
#define setup_videolfb_tag __setup_videolfb_tag
#else
#define setup_videolfb_tag(a) do {} while(0)
#endif

#ifdef CONFIG_REVISION_TAG
static void __setup_revision_tag(struct tag **in_params);
#define setup_revision_tag __setup_revision_tag
#else
#define setup_revision_tag(a) do {} while(0)
#endif

#ifdef CONFIG_SERIAL_TAG
void __setup_serial_tag(struct tag **tmp);
#define setup_serial tag __setup_serial_tag
#else
#define setup_serial_tag(a) do {} while(0)
#endif




#ifdef CONFIG_SHOW_BOOT_PROGRESS
# include <status_led.h>
# define SHOW_BOOT_PROGRESS(arg)	show_boot_progress(arg)
#else
# define SHOW_BOOT_PROGRESS(arg)
#endif

static int armlinux_architecture = 0;
static void *armlinux_bootparams = NULL;

void armlinux_set_bootparams(void *params)
{
	armlinux_bootparams = params;
}

void armlinux_set_architecture(int architecture)
{
	armlinux_architecture = architecture;
}

int do_bootm_linux(struct image_data *data)
{
	void (*theKernel)(int zero, int arch, void *params);
	image_header_t *os_header = &data->os->header;
	const char *commandline = getenv ("bootargs");

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

	theKernel = (void (*)(int, int, void *))ntohl((unsigned long)(os_header->ih_ep));

	debug ("## Transferring control to Linux (at address %08lx) ...\n",
	       (ulong) theKernel);

	setup_start_tag();
	setup_serial_tag(&params);
	setup_revision_tag(&params);
	setup_memory_tags();
	setup_commandline_tag(commandline);
#if 0
	if (initrd_start && initrd_end)
		setup_initrd_tag (initrd_start, initrd_end);
#endif
	setup_videolfb_tag((gd_t *) gd);
	setup_end_tag();

	if (relocate_image(data->os, (void *)ntohl(os_header->ih_load)))
		return -1;

	/* we assume that the kernel is in place */
	printf ("\nStarting kernel ...\n\n");

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

void
__setup_start_tag(void)
{
	params = (struct tag *)armlinux_bootparams;

	params->hdr.tag = ATAG_CORE;
	params->hdr.size = tag_size(tag_core);

	params->u.core.flags = 0;
	params->u.core.pagesize = 0;
	params->u.core.rootdev = 0;

	params = tag_next(params);
}


void
__setup_memory_tags(void)
{
	struct device_d *dev = NULL;

	list_for_each_entry(dev, &device_list, list) {
		if (dev->type == DEVICE_TYPE_DRAM) {
			params->hdr.tag = ATAG_MEM;
			params->hdr.size = tag_size(tag_mem32);

			params->u.mem.start = dev->map_base;
			params->u.mem.size = dev->size;

			params = tag_next(params);
		}
	}
}


void
__setup_commandline_tag(const char *commandline)
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


void
__setup_initrd_tag(ulong initrd_start, ulong initrd_end)
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

#if 0 /* FIXME: doesn't compile */
void
__setup_videolfb_tag(gd_t *gd)
{
	/* An ATAG_VIDEOLFB node tells the kernel where and how large
	 * the framebuffer for video was allocated (among other things).
	 * Note that a _physical_ address is passed !
	 *
	 * We only use it to pass the address and size, the other entries
	 * in the tag_videolfb are not of interest.
	 */
	params->hdr.tag = ATAG_VIDEOLFB;
	params->hdr.size = tag_size(tag_videolfb);

	params->u.videolfb.lfb_base = (u32)gd->fb_base;
	/* Fb size is calculated according to parameters for our panel */
	params->u.videolfb.lfb_size = calc_fbsize();

	params = tag_next(params);
}
#endif
#if 0 /* FIXME: doesn't compile */
void
__setup_serial_tag(struct tag **tmp)
{
	struct tag *params = *tmp;
	struct tag_serialnr serialnr;
	void get_board_serial(struct tag_serialnr *serialnr);

	get_board_serial(&serialnr);
	params->hdr.tag = ATAG_SERIAL;
	params->hdr.size = tag_size(tag_serialnr);
	params->u.serialnr.low = serialnr.low;
	params->u.serialnr.high = serialnr.high;
	params = tag_next(params);
	*tmp = params;
}
#endif
#if 0 /* FIXME: doesn't compile */
void
__setup_revision_tag(struct tag **in_params)
{
	u32 rev = 0;
	u32 get_board_rev(void);

	rev = get_board_rev();
	params->hdr.tag = ATAG_REVISION;
	params->hdr.size = tag_size(tag_revision);
	params->u.revision.rev = rev;
	params = tag_next(params);
}
#endif

void
__setup_end_tag (void)
{
	params->hdr.tag = ATAG_NONE;
	params->hdr.size = 0;
}

static int do_bootz(cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	void (*theKernel)(int zero, int arch, void *params);
	const char *commandline = getenv("bootargs");
	size_t size;

	if (argc != 2) {
		u_boot_cmd_usage(cmdtp);
		return 1;
	}

	theKernel = read_file(argv[1], &size);
	if (!theKernel) {
		printf("could not read %s\n", argv[1]);
		return 1;
	}

	printf("loaded zImage from %s with size %d\n", argv[1], size);

	setup_start_tag();
	setup_serial_tag(&params);
	setup_revision_tag(&params);
	setup_memory_tags();
	setup_commandline_tag(commandline);
#if 0
	if (initrd_start && initrd_end)
		setup_initrd_tag (initrd_start, initrd_end);
#endif
	setup_videolfb_tag((gd_t *) gd);
	setup_end_tag();

	cleanup_before_linux();
	theKernel (0, armlinux_architecture, armlinux_bootparams);

	return 0;
}

static const __maybe_unused char cmd_ls_help[] =
"Usage: bootz [FILE]\n"
"Boot a Linux zImage\n";

U_BOOT_CMD_START(bootz)
	.maxargs        = 2,
	.cmd            = do_bootz,
	.usage          = "bootz - start a zImage",
U_BOOT_CMD_END


