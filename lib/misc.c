/*
 * misc.c - various assorted functions
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <malloc.h>
#include <errno.h>
#include <fs.h>

unsigned long strtoul_suffix(const char *str, char **endp, int base)
{
        unsigned long val;
        char *end;

        val = simple_strtoul(str, &end, base);

        switch (*end) {
        case 'G':
                val *= 1024;
        case 'M':
		val *= 1024;
        case 'k':
        case 'K':
                val *= 1024;
                end++;
        }

        if (endp)
                *endp = (char *)end;

        return val;
}

int parse_area_spec(const char *str, ulong *start, ulong *size)
{
	char *endp;

	if (*str == '+') {
		/* no beginning given but size so start is 0 */
		*start = 0;
		*size = strtoul_suffix(str + 1, &endp, 0);
		return 0;
	}

	if (*str == '-') {
		/* no beginning given but end, so start is 0 */
		*start = 0;
		*size = strtoul_suffix(str + 1, &endp, 0) + 1;
		return 0;
	}

	*start = strtoul_suffix(str, &endp, 0);

	str = endp;

	if (!*str) {
		/* beginning given, but no size, assume maximum size */
		*size = ~0;
		return 0;
	}

	if (*str == '-') {
                /* beginning and end given */
		*size = strtoul_suffix(str + 1, NULL, 0) + 1;
		return 0;
	}

	if (*str == '+') {
                /* beginning and size given */
		*size = strtoul_suffix(str + 1, NULL, 0);
		return 0;
	}

	return -1;
}

int spec_str_to_info(const char *str, struct memarea_info *info)
{
	char *endp;
	int ret;

	info->device = device_from_spec_str(str, &endp);
	if (!info->device) {
		printf("unknown device: %s\n", deviceid_from_spec_str(str, NULL));
		return -ENODEV;
	}

	ret = parse_area_spec(endp, &info->start, &info->size);
	if (ret)
		return ret;

	if (info->size == ~0)
		info->size = info->device->size;

	return 0;
}

