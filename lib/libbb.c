/*
 * Utility routines.
 *
 * Copyright (C) many different people.
 * If you wrote this, please acknowledge your work.
 *
 * Licensed under GPLv2 or later, see file LICENSE in this tarball for details.
 */

#include <common.h>
#include <libbb.h>
#include <linux/stat.h>
#include <fs.h>
#include <xfuncs.h>
#include <malloc.h>
#include <environment.h>

/* concatenate path and file name to new allocation buffer,
 * not adding '/' if path name already has '/'
*/
char *concat_path_file(const char *path, const char *filename)
{
	char *lc, *str;

	if (!path)
		path = "";
	lc = last_char_is(path, '/');
	while (*filename == '/')
		filename++;

	str = xmalloc(strlen(path) + (lc==NULL ? 1 : 0) + strlen(filename) + 1);
	sprintf(str, "%s%s%s", path, (lc==NULL ? "/" : ""), filename);

	return str;
}
EXPORT_SYMBOL(concat_path_file);

/*
 * This function make special for recursive actions with usage
 * concat_path_file(path, filename)
 * and skipping "." and ".." directory entries
 */

char *concat_subpath_file(const char *path, const char *f)
{
	if (DOT_OR_DOTDOT(f))
		return NULL;
	return concat_path_file(path, f);
}
EXPORT_SYMBOL(concat_subpath_file);

/**
 * find_path - find a file in a colon separated path
 * @path: The search path, colon separated
 * @filename: The filename to search for
 * @filter: filter function
 *
 * searches for @filename in a colon separated list of directories given in
 * @path. @filter should return true when the current file matches the expectations,
 * false otherwise. @filter should check for existence of the file, but could also
 * check for additional flags.
 */
char *find_path(const char *path, const char *filename,
		bool (*filter)(const char *))
{
	char *p, *n, *freep;

	freep = p = strdup(path);
	while (p) {
		n = strchr(p, ':');
		if (n)
			*n++ = '\0';
		if (*p != '\0') { /* it's not a PATH="foo::bar" situation */
			p = concat_path_file(p, filename);
			if (filter(p)) {
				free(freep);
				return p;
			}
			free(p);
		}
		p = n;
	}
	free(freep);
	return NULL;
}
EXPORT_SYMBOL(find_path);

/* check if path points to an executable file;
 * return 1 if found;
 * return 0 otherwise;
 */
bool execable_file(const char *name)
{
	struct stat s;
	return (!stat(name, &s) && S_ISREG(s.st_mode));
}
EXPORT_SYMBOL(execable_file);


/* search $PATH for an executable file;
 * return allocated string containing full path if found;
 * return NULL otherwise;
 */
char *find_execable(const char *filename)
{
	return find_path(getenv("PATH"), filename, execable_file);
}
EXPORT_SYMBOL(find_execable);

/* Find out if the last character of a string matches the one given.
 * Don't underrun the buffer if the string length is 0.
 */
char* last_char_is(const char *s, int c)
{
	if (s && *s) {
		size_t sz = strlen(s) - 1;
		s += sz;
		if ( (unsigned char)*s == c)
			return (char*)s;
	}
	return NULL;
}
EXPORT_SYMBOL(last_char_is);

/* Like strncpy but make sure the resulting string is always 0 terminated. */
char * safe_strncpy(char *dst, const char *src, size_t size)
{
	if (!size) return dst;
	dst[--size] = '\0';
	return strncpy(dst, src, size);
}
EXPORT_SYMBOL(safe_strncpy);
