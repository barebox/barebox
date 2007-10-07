/*
 * (C) Copyright 2000-2002
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

#include <common.h>

/*
 * return a pointer to a string containing the size
 * as "xxx kB", "xxx.y kB", "xxx MB" or "xxx.y MB" as needed;
 */
char *size_human_readable(ulong size)
{
	static char buf[20];
	ulong m, n;
	ulong d = 1 << 20;		/* 1 MB */
	char  c = 'M';
	char *ptr = buf;

	if (size < d) {			/* print in kB */
		c = 'k';
		d = 1 << 10;
	}

	n = size / d;

	m = (10 * (size - (n * d)) + (d / 2) ) / d;

	if (m >= 10) {
		m -= 10;
		n += 1;
	}

	ptr += sprintf(buf, "%2ld", n);
	if (m) {
		ptr += sprintf (ptr,".%ld", m);
	}
	sprintf(ptr, " %cB", c);

	return buf;
}
EXPORT_SYMBOL(size_human_readable);
