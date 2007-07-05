/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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

/*
 * FLASH support
 */
#include <common.h>
#include <command.h>
#include <cfi_flash.h>

int do_flerase (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
        struct memarea_info mem;

	if (argc != 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}
# if 0
        if (spec_str_to_info(argv[1], &mem)) {
                printf("-ENOPARSE\n");
                return 1;
        }

        if (!mem.device->driver) {
                printf("This device has no driver\n");
                return 1;
        }
        if (!mem.device->driver->erase) {
                printf("This device cannot be erased\n");
                return 1;
        }

        dev_erase(mem.device, mem.size, mem.start);
#endif
#warning: do_flerase is broken
	return 0;
}

int do_protect (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int p;
	int rcode = 0;
	struct memarea_info mem_info;

	if (argc < 3) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	if (strcmp(argv[1], "off") == 0) {
		p = 0;
	} else if (strcmp(argv[1], "on") == 0) {
		p = 1;
	} else {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}
#if 0
	if (spec_str_to_info(argv[2], &mem_info)) {
		printf ("-EPARSE\n");
		return 1;
	}
#endif


//	rcode = flash_sect_protect (p, addr_first, addr_last);
	return rcode;
}

/**************************************************/

U_BOOT_CMD(
	erase,   2,   0,  do_flerase,
	"erase   - erase FLASH memory\n",
	"write me\n"
);

U_BOOT_CMD(
	protect,  4,  1,   do_protect,
	"protect - enable or disable FLASH write protection\n",
	"on  start end\n"
	"    - protect FLASH from addr 'start' to addr 'end'\n"
	"protect on start +len\n"
	"    - protect FLASH from addr 'start' to end of sect "
	"w/addr 'start'+'len'-1\n"
	"protect on  N:SF[-SL]\n"
	"    - protect sectors SF-SL in FLASH bank # N\n"
	"protect on  bank N\n    - protect FLASH bank # N\n"
	"protect on  all\n    - protect all FLASH banks\n"
	"protect off start end\n"
	"    - make FLASH from addr 'start' to addr 'end' writable\n"
	"protect off start +len\n"
	"    - make FLASH from addr 'start' to end of sect "
	"w/addr 'start'+'len'-1 wrtable\n"
	"protect off N:SF[-SL]\n"
	"    - make sectors SF-SL writable in FLASH bank # N\n"
	"protect off bank N\n    - make FLASH bank # N writable\n"
	"protect off all\n    - make all FLASH banks writable\n"
);
