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
 */

#include <boot.h>
#include <common.h>
#include <command.h>
#include <driver.h>
#include <environment.h>
#include <image.h>
#include <init.h>
#include <fs.h>
#include <linux/list.h>
#include <xfuncs.h>
#include <malloc.h>
#include <fcntl.h>
#include <errno.h>
#include <memory.h>
#include <of.h>
#include <magicvar.h>

#include <asm/byteorder.h>
#include <asm/setup.h>
#include <asm/barebox-arm.h>
#include <asm/armlinux.h>
#include <asm/system.h>

static struct tag *params;
static void *armlinux_bootparams = NULL;

static int armlinux_architecture;
static u32 armlinux_system_rev;
static u64 armlinux_system_serial;

BAREBOX_MAGICVAR(armlinux_architecture, "ARM machine ID");
BAREBOX_MAGICVAR(armlinux_system_rev, "ARM system revision");
BAREBOX_MAGICVAR(armlinux_system_serial, "ARM system serial");

void armlinux_set_architecture(int architecture)
{
	export_env_ull("armlinux_architecture", architecture);
	armlinux_architecture = architecture;
}

int armlinux_get_architecture(void)
{
	getenv_uint("armlinux_architecture", &armlinux_architecture);

	return armlinux_architecture;
}

void armlinux_set_revision(unsigned int rev)
{
	export_env_ull("armlinux_system_rev", rev);
	armlinux_system_rev = rev;
}

unsigned int armlinux_get_revision(void)
{
	getenv_uint("armlinux_system_rev", &armlinux_system_rev);

	return armlinux_system_rev;
}

void armlinux_set_serial(u64 serial)
{
	export_env_ull("armlinux_system_serial", serial);
	armlinux_system_serial = serial;
}

u64 armlinux_get_serial(void)
{
	getenv_ull("armlinux_system_serial", &armlinux_system_serial);

	return armlinux_system_serial;
}

#ifdef CONFIG_ARM_BOARD_APPEND_ATAG
static struct tag *(*atag_appender)(struct tag *);
void armlinux_set_atag_appender(struct tag *(*func)(struct tag *))
{
	atag_appender = func;
}
#endif

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

static void setup_memory_tags(void)
{
	struct memory_bank *bank;

	for_each_memory_bank(bank) {
		params->hdr.tag = ATAG_MEM;
		params->hdr.size = tag_size(tag_mem32);

		params->u.mem.start = bank->start;
		params->u.mem.size = bank->size;

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
	u32 system_rev = armlinux_get_revision();

	if (system_rev) {
		params->hdr.tag = ATAG_REVISION;
		params->hdr.size = tag_size(tag_revision);

		params->u.revision.rev = system_rev;

		params = tag_next(params);
	}
}

static void setup_serial_tag(void)
{
	u64 system_serial = armlinux_get_serial();

	if (system_serial) {
		params->hdr.tag = ATAG_SERIAL;
		params->hdr.size = tag_size(tag_serialnr);

		params->u.serialnr.low  = system_serial & 0xffffffff;
		params->u.serialnr.high = system_serial >> 32;

		params = tag_next(params);
	}
}

static void setup_initrd_tag(unsigned long start, unsigned long size)
{
	/* an ATAG_INITRD node tells the kernel where the compressed
	 * ramdisk can be found. ATAG_RDIMG is a better name, actually.
	 */
	params->hdr.tag = ATAG_INITRD2;
	params->hdr.size = tag_size(tag_initrd);

	params->u.initrd.start = start;
	params->u.initrd.size = size;

	params = tag_next(params);
}

static void setup_end_tag (void)
{
	params->hdr.tag = ATAG_NONE;
	params->hdr.size = 0;
}

static void setup_tags(unsigned long initrd_address,
		unsigned long initrd_size, int swap)
{
	const char *commandline = linux_bootargs_get();

	setup_start_tag();
	setup_memory_tags();
	setup_commandline_tag(commandline, swap);

	if (initrd_size)
		setup_initrd_tag(initrd_address, initrd_size);

	setup_revision_tag();
	setup_serial_tag();
#ifdef CONFIG_ARM_BOARD_APPEND_ATAG
	if (atag_appender != NULL)
		params = atag_appender(params);
#endif
	setup_end_tag();

	printf("commandline: %s\n"
	       "arch_number: %d\n", commandline, armlinux_get_architecture());

}

void armlinux_set_bootparams(void *params)
{
	armlinux_bootparams = params;
}

void start_linux(void *adr, int swap, unsigned long initrd_address,
		unsigned long initrd_size, void *oftree)
{
	void (*kernel)(int zero, int arch, void *params) = adr;
	void *params = NULL;
	int architecture;

	if (oftree) {
		printf("booting Linux kernel with devicetree\n");
		params = oftree;
	} else {
		setup_tags(initrd_address, initrd_size, swap);
		params = armlinux_bootparams;
	}
	architecture = armlinux_get_architecture();

	shutdown_barebox();
	if (swap) {
		u32 reg;
		__asm__ __volatile__("mrc p15, 0, %0, c1, c0" : "=r" (reg));
		reg ^= CR_B; /* swap big-endian flag */
		__asm__ __volatile__("mcr p15, 0, %0, c1, c0" :: "r" (reg));
	}

#ifdef CONFIG_THUMB2_BAREBOX
	__asm__ __volatile__ (
		"mov r0, #0\n"
		"mov r1, %0\n"
		"mov r2, %1\n"
		"bx %2\n"
		:
		: "r" (architecture), "r" (params), "r" (kernel)
		: "r0", "r1", "r2"
	);
#else
	kernel(0, architecture, params);
#endif
}
