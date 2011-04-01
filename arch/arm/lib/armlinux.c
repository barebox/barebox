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
#include <errno.h>

#include <asm/byteorder.h>
#include <asm/global_data.h>
#include <asm/setup.h>
#include <asm/barebox-arm.h>
#include <asm/armlinux.h>
#include <asm/system.h>

static struct tag *params;
static int armlinux_architecture = 0;
static void *armlinux_bootparams = NULL;

static unsigned int system_rev;
static u64 system_serial;

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

static void setup_commandline_tag(const char *commandline, int swap)
{
	const char *p;
	size_t words;

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

	words = (strlen(p) + 1 /* NUL */ + 3 /* round up */) >> 2;
	params->hdr.tag = ATAG_CMDLINE;
	params->hdr.size = (sizeof(struct tag_header) >> 2) + words;

	strcpy(params->u.cmdline.cmdline, p);

#ifdef CONFIG_BOOT_ENDIANNESS_SWITCH
	if (swap) {
		u32 *cmd = (u32 *)params->u.cmdline.cmdline;
		while (words--)
			cmd[words] = swab32(cmd[words]);
	}
#endif

	params = tag_next(params);
}

static void setup_revision_tag(void)
{
	if (system_rev) {
		params->hdr.tag = ATAG_REVISION;
		params->hdr.size = tag_size(tag_revision);

		params->u.revision.rev = system_rev;

		params = tag_next(params);
	}
}

static void setup_serial_tag(void)
{
	if (system_serial) {
		params->hdr.tag = ATAG_SERIAL;
		params->hdr.size = tag_size(tag_serialnr);

		params->u.serialnr.low  = system_serial & 0xffffffff;
		params->u.serialnr.high = system_serial >> 32;

		params = tag_next(params);
	}
}

static void setup_initrd_tag(image_header_t *header)
{
	/* an ATAG_INITRD node tells the kernel where the compressed
	 * ramdisk can be found. ATAG_RDIMG is a better name, actually.
	 */
	params->hdr.tag = ATAG_INITRD2;
	params->hdr.size = tag_size(tag_initrd);

	params->u.initrd.start = image_get_load(header);
	params->u.initrd.size = image_get_data_size(header);

	params = tag_next(params);
}

static void setup_end_tag (void)
{
	params->hdr.tag = ATAG_NONE;
	params->hdr.size = 0;
}

static void setup_tags(struct image_data *data, int swap)
{
	const char *commandline = getenv("bootargs");

	setup_start_tag();
	setup_memory_tags();
	setup_commandline_tag(commandline, swap);

	if (data && data->initrd)
		setup_initrd_tag (&data->initrd->header);

	setup_revision_tag();
	setup_serial_tag();
	setup_end_tag();

	printf("commandline: %s\n"
	       "arch_number: %d\n", commandline, armlinux_architecture);

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

void armlinux_set_revision(unsigned int rev)
{
	system_rev = rev;
}

void armlinux_set_serial(u64 serial)
{
	system_serial = serial;
}

void start_linux(void *adr, int swap, struct image_data *data)
{
	void (*kernel)(int zero, int arch, void *params) = adr;

	setup_tags(data, swap);

	shutdown_barebox();
	if (swap) {
		u32 reg;
		__asm__ __volatile__("mrc p15, 0, %0, c1, c0" : "=r" (reg));
		reg ^= CR_B; /* swap big-endian flag */
		__asm__ __volatile__("mcr p15, 0, %0, c1, c0" :: "r" (reg));
	}

	kernel(0, armlinux_architecture, armlinux_bootparams);
}
