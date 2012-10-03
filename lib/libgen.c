/*
 * libgen.c - some libgen functions needed for barebox
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
 */

#include <common.h>
#include <libgen.h>

char *basename (char *path)
{
	char *fname;

	if(!strchr(path, '/'))
		return path;

	fname = path + strlen(path) - 1;
	while (fname >= path) {
		if (*fname == '/') {
			fname++;
			break;
		}
		fname--;
	}
	return fname;
}
EXPORT_SYMBOL(basename);

char *dirname (char *path)
{
	char *fname;
	static char str[2];

	if(!strchr(path, '/')) {
		str[0] = '.';
		str[1] = 0;
		return str;
	}

	fname = basename (path);
	--fname;
	if (path == fname)
		*fname++ = '/';
	*fname = '\0';
	return path;
}
EXPORT_SYMBOL(dirname);
