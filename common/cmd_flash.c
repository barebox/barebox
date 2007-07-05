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
#include <errno.h>

int do_flerase (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
        struct memarea_info mem;

	if (argc != 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

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

        if(dev_erase(mem.device, mem.size, mem.start)) {
		perror("erase");
		return 1;
	}

	return 0;
}

U_BOOT_CMD_START(erase)
	.maxargs	= 2,
	.cmd		= do_flerase,
	.usage		= "erase   - erase FLASH memory\n",
	U_BOOT_CMD_HELP("write me\n")
U_BOOT_CMD_END

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

	if (spec_str_to_info(argv[2], &mem_info)) {
		printf ("-EPARSE\n");
		return 1;
	}

//	rcode = flash_sect_protect (p, addr_first, addr_last);
	return rcode;
}

U_BOOT_CMD_START(protect)
	.maxargs	= 4,
	.cmd		= do_protect,
	.usage		= "protect - enable or disable FLASH write protection\n",
	U_BOOT_CMD_HELP("write me\n")
U_BOOT_CMD_END
