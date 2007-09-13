/*
 * U-boot - blackfin_linux.c
 *
 * Copyright (c) 2005 blackfin.uclinux.org
 *
 * (C) Copyright 2000-2004
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

/* Dummy functions, currently not in Use */

#include <common.h>
#include <command.h>
#include <image.h>
#include <environment.h>
#include <asm/byteorder.h>
#include <asm/cpu.h>

#define CMD_LINE_ADDR 0xFF900000  /* L1 scratchpad */

int do_bootm_linux(struct image_handle *os_handle, struct image_handle *initrd)
{
	int (*appl)(char *cmdline);
	const char *cmdline = getenv("bootargs");
	char *cmdlinedest = (char *) CMD_LINE_ADDR;
	image_header_t *os_header = &os_handle->header;

	appl = (int (*)(char *))ntohl(os_header->ih_ep);
	printf("Starting Kernel at = %x\n", appl);

	strncpy(cmdlinedest, cmdline, 0x1000);
	cmdlinedest[0xfff] = 0;

	if(icache_status()){
		flush_instruction_cache();
		icache_disable();
	}

	if(dcache_status()){
		flush_data_cache();
		dcache_disable();
	}

	(*appl)(cmdlinedest);

	return -1;
}

