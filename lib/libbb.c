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
	if (f && DOT_OR_DOTDOT(f))
		return NULL;
	return concat_path_file(path, f);
}
EXPORT_SYMBOL(concat_subpath_file);

/* check if path points to an executable file;
 * return 1 if found;
 * return 0 otherwise;
 */
int execable_file(const char *name)
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

char *simple_itoa(unsigned int i)
{
	/* 21 digits plus null terminator, good for 64-bit or smaller ints */
	static char local[22];
	char *p = &local[21];
	*p-- = '\0';
	do {
		*p-- = '0' + i % 10;
		i /= 10;
	} while (i > 0);
	return p + 1;
}
EXPORT_SYMBOL(simple_itoa);

/*
 * write_full - write to filedescriptor
 *
 * Like write, but guarantees to write the full buffer out, else
 * it returns with an error.
 */
int write_full(int fd, void *buf, size_t size)
{
	size_t insize = size;
	int now;

	while (size) {
		now = write(fd, buf, size);
		if (now <= 0)
			return now;
		size -= now;
		buf += now;
	}

	return insize;
}
EXPORT_SYMBOL(write_full);

/*
 * read_full - read from filedescriptor
 *
 * Like read, but this function only returns less bytes than
 * requested when the end of file is reached.
 */
int read_full(int fd, void *buf, size_t size)
{
	size_t insize = size;
	int now;
	int total = 0;

	while (size) {
		now = read(fd, buf, size);
		if (now == 0)
			return total;
		if (now < 0)
			return now;
		total += now;
		size -= now;
		buf += now;
	}

	return insize;
}
EXPORT_SYMBOL(read_full);

/*
 * read_file_line - read a line from a file
 *
 * Used to compose a filename from a printf format and to read a line from this
 * file. All leading and trailing whitespaces (including line endings) are
 * removed. The returned buffer must be freed with free(). This function is
 * supposed for reading variable like content into a buffer, so files > 1024
 * bytes are ignored.
 */
char *read_file_line(const char *fmt, ...)
{
	va_list args;
	char *filename;
	char *buf, *line = NULL;
	size_t size;
	int ret;
	struct stat s;

	va_start(args, fmt);
	filename = vasprintf(fmt, args);
	va_end(args);

	ret = stat(filename, &s);
	if (ret)
		goto out;

	if (s.st_size > 1024)
		goto out;

	buf = read_file(filename, &size);
	if (!buf)
		goto out;

	line = strim(buf);

	line = xstrdup(line);
	free(buf);
out:
	free(filename);
	return line;
}
EXPORT_SYMBOL_GPL(read_file_line);
