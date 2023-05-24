// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2012 Jan Luebbe <j.luebbe@pengutronix.de>, Pengutronix
/*
 * mmuinfo_32.c - Show MMU/cache information from cp15 registers
 */

#include <common.h>
#include <asm/mmuinfo.h>

static char *inner_attr[] = {
	"0b000 Non-cacheable",
	"0b001 Strongly-ordered",
	"0b010 (reserved)",
	"0b011 Device",
	"0b100 (reserved)",
	"0b101 Write-Back, Write-Allocate",
	"0b110 Write-Through",
	"0b111 Write-Back, no Write-Allocate",
};

static char *outer_attr[] = {
	"0b00 Non-cacheable",
	"0b01 Write-Back, Write-Allocate",
	"0b10 Write-Through, no Write-Allocate",
	"0b11 Write-Back, no Write-Allocate",
};

static void decode_par(unsigned long par)
{
	printf("  Physical Address [31:12]: 0x%08lx\n", par & 0xFFFFF000);
	printf("  Reserved [11]:            0x%lx\n", (par >> 11) & 0x1);
	printf("  Not Outer Shareable [10]: 0x%lx\n", (par >> 10) & 0x1);
	printf("  Non-Secure [9]:           0x%lx\n", (par >> 9) & 0x1);
	printf("  Impl. def. [8]:           0x%lx\n", (par >> 8) & 0x1);
	printf("  Shareable [7]:            0x%lx\n", (par >> 7) & 0x1);
	printf("  Inner mem. attr. [6:4]:   0x%lx (%s)\n", (par >> 4) & 0x7,
		inner_attr[(par >> 4) & 0x7]);
	printf("  Outer mem. attr. [3:2]:   0x%lx (%s)\n", (par >> 2) & 0x3,
		outer_attr[(par >> 2) & 0x3]);
	printf("  SuperSection [1]:         0x%lx\n", (par >> 1) & 0x1);
	printf("  Failure [0]:              0x%lx\n", (par >> 0) & 0x1);
}

int mmuinfo_v7(void *_addr)
{
	unsigned long addr = (unsigned long)_addr;
	unsigned long priv_read, priv_write;

	__asm__ __volatile__(
		"mcr    p15, 0, %0, c7, c8, 0   @ write VA to PA translation (priv read)\n"
		:
		: "r" (addr)
		: "memory");

	__asm__ __volatile__(
		"mrc    p15, 0, %0, c7, c4, 0   @ read PAR\n"
		: "=r" (priv_read)
		:
		: "memory");

	__asm__ __volatile__(
		"mcr    p15, 0, %0, c7, c8, 1   @ write VA to PA translation (priv write)\n"
		:
		: "r" (addr)
		: "memory");

	__asm__ __volatile__(
		"mrc    p15, 0, %0, c7, c4, 0   @ read PAR\n"
		: "=r" (priv_write)
		:
		: "memory");

	printf("PAR result for 0x%08lx: \n", addr);
	printf(" privileged read: 0x%08lx\n", priv_read);
	decode_par(priv_read);
	printf(" privileged write: 0x%08lx\n", priv_write);
	decode_par(priv_write);

	return 0;
}
