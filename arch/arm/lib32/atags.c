// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2002 Sysgo Real-Time Solutions GmbH (http://www.elinos.com, Marius Groeger <mgroeger@sysgo.de>)
// SPDX-FileCopyrightText: 2001 Erik Mouw <J.A.K.Mouw@its.tudelft.nl>

#include <bootargs.h>
#include <common.h>
#include <environment.h>
#include <xfuncs.h>
#include <magicvar.h>

#include <asm/setup.h>
#include <asm/armlinux.h>

static struct tag *params;
static void *armlinux_bootparams = NULL;

static unsigned armlinux_architecture;
static u32 armlinux_system_rev;
static u64 armlinux_system_serial;

BAREBOX_MAGICVAR(armlinux_architecture, "ARM machine ID");
BAREBOX_MAGICVAR(armlinux_system_rev, "ARM system revision");
BAREBOX_MAGICVAR(armlinux_system_serial, "ARM system serial");

void armlinux_set_architecture(unsigned architecture)
{
	export_env_ull("armlinux_architecture", architecture);
	armlinux_architecture = architecture;
}

unsigned armlinux_get_architecture(void)
{
	getenv_uint("armlinux_architecture", &armlinux_architecture);

	return armlinux_architecture;
}

void armlinux_set_revision(unsigned int rev)
{
	export_env_ull("armlinux_system_rev", rev);
	armlinux_system_rev = rev;
}

static unsigned int armlinux_get_revision(void)
{
	getenv_uint("armlinux_system_rev", &armlinux_system_rev);

	return armlinux_system_rev;
}

void armlinux_set_serial(u64 serial)
{
	export_env_ull("armlinux_system_serial", serial);
	armlinux_system_serial = serial;
}

static u64 armlinux_get_serial(void)
{
	getenv_ull("armlinux_system_serial", &armlinux_system_serial);

	return armlinux_system_serial;
}

void armlinux_set_bootparams(void *params)
{
	armlinux_bootparams = params;
}

struct tag *armlinux_get_bootparams(void)
{
	struct memory_bank *mem;

	if (armlinux_bootparams)
		return armlinux_bootparams;

	for_each_memory_bank(mem)
		return (void *)mem->start + 0x100;

	BUG();
}

static struct tag *(*atag_appender)(struct tag *);

#if defined CONFIG_ARM_BOARD_APPEND_ATAG
void armlinux_set_atag_appender(struct tag *(*func)(struct tag *))
{
	atag_appender = func;
}
#endif

static void setup_start_tag(void)
{
	params = armlinux_get_bootparams();

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

void armlinux_setup_tags(unsigned long initrd_address,
			 unsigned long initrd_size, int swap)
{
	const char *commandline = linux_bootargs_get();

	setup_start_tag();
	if (IS_ENABLED(CONFIG_ARM_BOARD_PREPEND_ATAG) && atag_appender)
		params = atag_appender(params);

	setup_memory_tags();
	setup_commandline_tag(commandline, swap);

	if (initrd_size)
		setup_initrd_tag(initrd_address, initrd_size);

	setup_revision_tag();
	setup_serial_tag();
	if (IS_ENABLED(CONFIG_ARM_BOARD_APPEND_ATAG) && atag_appender &&
			!IS_ENABLED(CONFIG_ARM_BOARD_PREPEND_ATAG))
		params = atag_appender(params);

	setup_end_tag();

	printf("commandline: %s\n"
	       "arch_number: %u\n", commandline, armlinux_get_architecture());

}
