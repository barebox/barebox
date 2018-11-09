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
#include <linux/string.h>

char *basename (char *path)
{
	return (char *)kbasename(path);
}
EXPORT_SYMBOL(basename);

/*
 * There are two different versions of basename(): The GNU version implemented
 * above and the POSIX version. The GNU version never modifies its argument and
 * returns the empty string when path has a trailing slash, and in particular
 * also when it is "/".
 */
char *posix_basename(char *path)
{
	char *fname;

	fname = path + strlen(path) - 1;

	while (*fname == '/') {
		if (fname == path)
			return path;
		*fname = '\0';
		fname--;
	}

	return basename(path);
}
EXPORT_SYMBOL(posix_basename);

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
