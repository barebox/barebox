/*
 * cpuinfo.c - Show information about cp15 registers
 *
 * Copyright (c) 2009 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <command.h>

static void decode_cache(unsigned long size)
{
	int linelen = 1 << ((size & 0x3) + 3);
	int mult = 2 + ((size >> 2) & 1);
	int cache_size = mult << (((size >> 6) & 0x7) + 8);

	if (((size >> 2) & 0xf) == 1)
		printf("no cache\n");
	else
		printf("%d bytes (linelen = %d)\n", cache_size, linelen);
}

static char *post_arm7_archs[] = {"v4", "v4T", "v5", "v5T", "v5TE", "v5TEJ", "v6"};

static char *crbits[] = {"M", "A", "C", "W", "P", "D", "L", "B", "S", "R",
	"F", "Z", "I", "V", "RR", "L4", "", "", "", "", "", "FI", "U", "XP",
	"VE", "EE", "L2"};

static int do_cpuinfo(struct command *cmdtp, int argc, char *argv[])
{
	unsigned long mainid, cache, cr;
	char *architecture, *implementer;
	int i;

	__asm__ __volatile__(
		"mrc    p15, 0, %0, c0, c0, 0   @ read control reg\n"
		: "=r" (mainid)
		:
		: "memory");

	__asm__ __volatile__(
		"mrc    p15, 0, %0, c0, c0, 1   @ read control reg\n"
		: "=r" (cache)
		:
		: "memory");

	__asm__ __volatile__(
		"mrc    p15, 0, %0, c1, c0, 0   @ read control reg\n"
			: "=r" (cr)
			:
			: "memory");

	switch (mainid >> 24) {
	case 0x41:
		implementer = "ARM";
		break;
	case 0x44:
		implementer = "Digital Equipment Corp.";
		break;
	case 0x40:
		implementer = "Motorola - Freescale Semiconductor Inc.";
		break;
	case 0x56:
		implementer = "Marvell Semiconductor Inc.";
		break;
	case 0x69:
		implementer = "Intel Corp.";
		break;
	default:
		implementer = "Unknown";
	}

	if ((mainid & 0x0008f000) == 0x00000000) {
                /* pre-ARM7 */
                architecture = "Pre-ARM7";
        } else {
                if ((mainid & 0x0008f000) == 0x00007000) {
                        /* ARM7 */
			if (mainid & (1 << 23))
				architecture = "3";
			else
				architecture = "4T";
                } else {
                        /* post-ARM7 */
			int arch = (mainid >> 16) & 0xf;
			if (arch > 0 && arch < 8)
				architecture = post_arm7_archs[arch - 1];
			else
				architecture = "Unknown";
                }
        }

	printf("implementer: %s\narchitecture: %s\n",
			implementer, architecture);

	if (cache & (1 << 24)) {
		/* seperate I/D cache */
		printf("I-cache: ");
		decode_cache(cache & 0xfff);
		printf("D-cache: ");
		decode_cache((cache >> 12) & 0xfff);
	} else {
		/* unified I/D cache */
		printf("cache: ");
		decode_cache(cache & 0xfff);
	}

	printf("Control register: ");
	for (i = 0; i < 27; i++)
		if (cr & (1 << i))
			printf("%s ", crbits[i]);
	printf("\n");

	return 0;
}

BAREBOX_CMD_START(cpuinfo)
	.cmd            = do_cpuinfo,
	.usage          = "Show info about CPU",
BAREBOX_CMD_END

