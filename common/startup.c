/*
 * (C) Copyright 2002-2006
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <init.h>
#include <command.h>
#include <malloc.h>
#include <linux/utsrelease.h>
#include <mem_malloc.h>
#include <debug_ll.h>
#include <fs.h>
#include <linux/stat.h>
#include <reloc.h>

#ifndef CONFIG_IDENT_STRING
#define CONFIG_IDENT_STRING ""
#endif

extern initcall_t __u_boot_initcalls_start[], __u_boot_early_initcalls_end[], __u_boot_initcalls_end[];

const char *version_string =
	"U-Boot " UTS_RELEASE " (" __DATE__ " - " __TIME__ ")"CONFIG_IDENT_STRING;

static int display_banner (void)
{
	const char *vers = RELOC_VAR(version_string);

	printf (RELOC("\n\n%s\n\n"), RELOC(vers));
	debug (RELOC("U-Boot code: %08lX -> %08lX  BSS: -> %08lX\n"),
	       _u_boot_start, _bss_start, _bss_end);
	printf(RELOC("Board: " CONFIG_BOARDINFO "\n"));

	return 0;
}

#ifdef CONFIG_HAS_EARLY_INIT

#define EARLY_INITDATA (CFG_INIT_RAM_ADDR + CFG_INIT_RAM_SIZE \
		- CONFIG_EARLY_INITDATA_SIZE)

void *init_data_ptr = (void *)EARLY_INITDATA;

void early_init (void)
{
	/* copy the early initdata segment to early init RAM */
	memcpy((void *)EARLY_INITDATA, RELOC(&__early_init_data_begin),
				(ulong)&__early_init_data_size);
	early_console_start(RELOC("psc3"), 115200);

	display_banner();
}

#endif /* CONFIG_HAS_EARLY_INIT */

void start_uboot (void)
{
        initcall_t *initcall;
        int result;
	struct stat s;

#ifdef CONFIG_HAS_EARLY_INIT
	/* We are running from RAM now, copy early initdata from
	 * early RAM to RAM
	 */
	memcpy(&__early_init_data_begin, init_data_ptr,
			(ulong)&__early_init_data_size);
	init_data_ptr = &__early_init_data_begin;
#endif /* CONFIG_HAS_EARLY_INIT */

	for (initcall = __u_boot_initcalls_start;
			initcall < __u_boot_initcalls_end; initcall++) {
		PUTHEX_LL(*initcall);
		PUTC_LL('\n');
                result = (*initcall)();
                if (result)
                        hang();
        }

#ifndef CONFIG_HAS_EARLY_INIT
	display_banner();
#endif

	mount("none", "ramfs", "/");
	mkdir("/dev");
	mkdir("/env");
	mount("none", "devfs", "/dev");
	run_command("loadenv", 0);

	if (!stat("/env/init", &s)) {
		printf("running /env/init\n");
		run_command("exec /env/init", 0);
	}

        /* main_loop() can return to retry autoboot, if so just run it again. */
	for (;;) {
		main_loop ();
	}

	/* NOTREACHED - no way out of command loop except booting */
}

void hang (void)
{
	puts ("### ERROR ### Please RESET the board ###\n");
	for (;;);
}
