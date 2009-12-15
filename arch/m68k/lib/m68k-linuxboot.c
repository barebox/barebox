/*
 * Copyright (c) 2008 Carsten Schlote <c.schlote@konzeptpark.de>
 * See file CREDITS for list of people who contributed to this project.
 *
 * This file is part of barebox.
 *
 * barebox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * barebox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with barebox.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  @brief Linux boot preparation code.
 *
 * This file is responsible to start a linux kernel on
 * Coldfire targets.
 *
 * @note Only Colilo mode supported yet.
 */
#include <common.h>
#include <command.h>
#include <driver.h>
#include <image.h>
#include <zlib.h>
#include <init.h>

#include <asm/byteorder.h>
#include <asm/setup.h>
#include <environment.h>
#include <boot.h>
#include <asm/barebox-m68k.h>
#include <asm/bootinfo.h>


static int m68k_architecture = MACH_TYPE_GENERIC;


/*
 *  Setup M68k/Coldfire bootrecord info
 */
#if defined (CONFIG_SETUP_MEMORY_TAGS) || \
    defined (CONFIG_CMDLINE_TAG) || \
    defined (CONFIG_INITRD_TAG)


static void setup_boot_record(char* start_boot_rec, const char* command_line)
{
	struct bi_record* record;

	*start_boot_rec++ = 'C';
	*start_boot_rec++ = 'o';
	*start_boot_rec++ = 'L';
	*start_boot_rec++ = 'i';
	*start_boot_rec++ = 'L';
	*start_boot_rec++ = 'o';

	record = (struct bi_record*) start_boot_rec;

	/* specify memory layout */
#ifdef CONFIG_SETUP_MEMORY_TAGS
	record->tag = BI_MEMCHUNK;
	record->data[0] = 0;
	record->data[1] = 64 * 1024 * 1024; // TODO: to be changed for different boards
	record->size =  sizeof (record->tag) + sizeof (record->size)
		+ sizeof (record->data[0]) + sizeof (record->data[0]);
	record = (struct bi_record *) ((void *) record + record->size);
#endif

	/* add a kernel command line  */
#ifdef CONFIG_CMDLINE_TAG
	record->tag = BI_COMMAND_LINE;
	strcpy ((char *) &record->data, command_line);
	record->size = sizeof (record->tag) + sizeof (record->size)
		+ max (sizeof (record->data[0]), strlen (command_line)+1);
	record = (struct bi_record *) ((void *) record + record->size);
#endif

	/* Add reference to initrd */
#ifdef CONFIG_INITRD_TAG
#endif

	/* Mark end of tags  */
	record->tag = 0;
	record->data[0] = 0;
	record->data[1] = 0;
	record->size = sizeof(record->tag) + sizeof (record->size)
		    + sizeof (record->data[0]) + sizeof (record->data[0]);
}

#else
#define setup_boot_record(start_boot_rec,command_line) while (0) { }

#endif /* CONFIG_SETUP_MEMORY_TAGS || CONFIG_CMDLINE_TAG || CONFIG_INITRD_TAG */


static int do_bootm_linux(struct image_data *data)
{
	image_header_t *os_header = &data->os->header;
	void (*theKernel)(int zero, int arch, uint params);
	const char *commandline = getenv ("bootargs");
	uint32_t loadaddr,loadsize;

	if (os_header->ih_type == IH_TYPE_MULTI) {
		printf("Multifile images not handled at the moment\n");
		return -1;
	}

	printf("commandline: %s\n", commandline);

	theKernel = (void (*)(int,int,uint))ntohl((os_header->ih_ep));

	debug ("## Transferring control to Linux (at address %08lx) ...\n",
	       (ulong) theKernel);

	loadaddr = (uint32_t)ntohl(os_header->ih_load);
	loadsize = (uint32_t)ntohl(os_header->ih_size);
	setup_boot_record( (char*)(loadaddr+loadsize),(char*)commandline);

	if (relocate_image(data->os, (void *)loadaddr))
		return -1;

	/* we assume that the kernel is in place */
	printf ("\nStarting kernel image at 0x%08x size 0x%08x eentry 0x%08x\n\n",
		loadaddr, loadsize, (ulong) theKernel);

	/* Bring board into inactive post-reset state again */
	cleanup_before_linux ();

	/* Jump to kernel entry point */
	theKernel (0, m68k_architecture, 0xdeadbeaf);

	enable_interrupts();
	printf("Error: Loaded kernel returned. Probably it couldn't\n"
	       "find it's bootrecord.\n");
	return -1;
}

/*
 *  Register handler for m68k Kernel Images
 */
static int image_handle_cmdline_parse(struct image_data *data, int opt,	char *optarg)
{
	switch (opt)
	{
	case 'a':
		m68k_architecture = simple_strtoul(optarg, NULL, 0);
		return 0;
	default:
		return 1;
	}
}

static struct image_handler handler =
{
	.cmdline_options = "a:",
	.cmdline_parse = image_handle_cmdline_parse,
	.help_string = " -a <arch>      use architecture number <arch>",

	.bootm = do_bootm_linux,
	.image_type = IH_OS_LINUX,
};

static int m68klinux_register_image_handler(void)
{
	return register_image_handler(&handler);
}

late_initcall(m68klinux_register_image_handler);
