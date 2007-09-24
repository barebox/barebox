/* vi: set sw=8 ts=8: */
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

	str = xmalloc(strlen(path) + (lc==0 ? 1 : 0) + strlen(filename) + 1);
	sprintf(str, "%s%s%s", path, (lc==NULL ? "/" : ""), filename);

	return str;
}

/*
 * This function make special for recursive actions with usage
 * concat_path_file(path, filename)
 * and skipping "." and ".." directory entries
 */

char *concat_subpath_file(const char *path, const char *f)
{
	if (f && DOT_OR_DOTDOT(f))
		return NULL;
	return concat_path_file(path, f);
}

/* check if path points to an executable file;
 * return 1 if found;
 * return 0 otherwise;
 */
int execable_file(const char *name)
{
	struct stat s;
	return (!stat(name, &s) && S_ISREG(s.st_mode));
}

/* search $PATH for an executable file;
 * return allocated string containing full path if found;
 * return NULL otherwise;
 */
char *find_execable(const char *filename)
{
	char *path, *p, *n;

	p = path = strdup(getenv("PATH"));
	while (p) {
		n = strchr(p, ':');
		if (n)
			*n++ = '\0';
		if (*p != '\0') { /* it's not a PATH="foo::bar" situation */
			p = concat_path_file(p, filename);
			if (execable_file(p)) {
				free(path);
				return p;
			}
			free(p);
		}
		p = n;
	}
	free(path);
	return NULL;
}

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

