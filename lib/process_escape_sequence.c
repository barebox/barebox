/*
 * process_esacpe_sequence.c
 *
 * Copyright (c) 2010 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <fs.h>
#include <libbb.h>

int process_escape_sequence(const char *source, char *dest, int destlen)
{
	int i = 0;

	while (*source) {
		if (*source == '\\') {
			switch (*(source + 1)) {
			case 0:
				return 0;
			case '\\':
				dest[i++] = '\\';
				break;
			case 'a':
				dest[i++] = '\a';
				break;
			case 'b':
				dest[i++] = '\b';
				break;
			case 'n':
				dest[i++] = '\n';
				break;
			case 'r':
				dest[i++] = '\r';
				break;
			case 't':
				dest[i++] = '\t';
				break;
			case 'f':
				dest[i++] = '\f';
				break;
			case 'e':
				dest[i++] = 0x1b;
				break;
			case 'h':
				i += snprintf(dest + i, destlen - i, "%s", CONFIG_BOARDINFO);
				break;
			case 'w':
				i += snprintf(dest + i, destlen - i, "%s", getcwd());
				break;
			default:
				dest[i++] = '\\';
				dest[i++] = *(source + 1);
			}
			source++;
		} else
			dest[i++] = *source;
		source++;
		if (!(destlen - i))
			break;
	}
	dest[i] = 0;
	return 0;
}

