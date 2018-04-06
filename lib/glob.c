/* Copyright (C) 1991, 1992, 1993, 1994, 1995 Free Software Foundation, Inc.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.  */

#include <common.h>
#include <errno.h>
#include <fs.h>
#include <linux/stat.h>
#include <malloc.h>
#include <xfuncs.h>
#include <fnmatch.h>
#include <qsort.h>
#define _GNU_SOURCE
#include <glob.h>

#ifdef CONFIG_GLOB

extern __ptr_t(*__glob_opendir_hook) __P((const char *directory));
extern void (*__glob_closedir_hook) __P((__ptr_t stream));
extern const char *(*__glob_readdir_hook) __P((__ptr_t stream));

static int glob_in_dir __P((const char *pattern, const char *directory,
			    int flags,
			    int (*errfunc) __P((const char *, int)),
			    glob_t * pglob));
static int prefix_array __P((const char *prefix, char **array, size_t n,
			     int add_slash));

#ifdef __GLOB64
extern int glob_pattern_p(const char *pattern, int quote);
#else
/* Return nonzero if PATTERN contains any metacharacters.
   Metacharacters can be quoted with backslashes if QUOTE is nonzero.  */
int glob_pattern_p(const char *pattern, int quote)
{
	const char *p;
	int open = 0;

	for (p = pattern; *p != '\0'; ++p)
		switch (*p) {
		case '?':
		case '*':
			return 1;

		case '\\':
			if (quote && p[1] != '\0')
				++p;
			break;

		case '[':
			open = 1;
			break;

		case ']':
			if (open)
				return 1;
			break;
		}

	return 0;
}
#endif

#ifdef CONFIG_GLOB_SORT
/* Do a collated comparison of A and B.  */
static int collated_compare(const void *a, const void *b)
{
	const char *const s1 = *(const char *const *)a;
	const char *const s2 = *(const char *const *)b;

	if (s1 == s2)
		return 0;
	if (s1 == NULL)
		return 1;
	if (s2 == NULL)
		return -1;
	return strcmp(s1, s2);
}
#endif

/* Do glob searching for PATTERN, placing results in PGLOB.
   The bits defined above may be set in FLAGS.
   If a directory cannot be opened or read and ERRFUNC is not nil,
   it is called with the pathname that caused the error, and the
   `errno' value from the failing call; if it returns non-zero
   `glob' returns GLOB_ABEND; if it returns zero, the error is ignored.
   If memory cannot be allocated for PGLOB, GLOB_NOSPACE is returned.
   Otherwise, `glob' returns zero.  */
int glob(const char *pattern, int flags,
		int (*errfunc) __P((const char *, int)), glob_t *pglob)
{
	const char *filename;
	char *dirname = NULL;
	size_t dirlen;
	int status;
	int oldcount;

	if (pattern == NULL || pglob == NULL || (flags & ~__GLOB_FLAGS) != 0) {
		errno = EINVAL;
		return -1;
	}

	/* Find the filename.  */
	filename = strrchr(pattern, '/');
	if (filename == NULL) {
		filename = pattern;
		dirname = strdup(".");
		dirlen = 0;
	} else if (filename == pattern) {
		/* "/pattern".  */
		dirname = strdup("/");
		dirlen = 1;
		++filename;
	} else {
		dirlen = filename - pattern;
		dirname = (char *)xmalloc(dirlen + 1);
		memcpy(dirname, pattern, dirlen);
		dirname[dirlen] = '\0';
		++filename;
	}

	if (filename[0] == '\0' && dirlen > 1) {
		/* "pattern/".  Expand "pattern", appending slashes.  */
		int val = glob(dirname, flags | GLOB_MARK, errfunc, pglob);
		if (val == 0)
			pglob->gl_flags =
			    (pglob->
			     gl_flags & ~GLOB_MARK) | (flags & GLOB_MARK);
		status = val;
		goto out;
	}

	if (!(flags & GLOB_APPEND)) {
		pglob->gl_pathc = 0;
		pglob->gl_pathv = NULL;
	}

	oldcount = pglob->gl_pathc;

	if (glob_pattern_p(dirname, !(flags & GLOB_NOESCAPE))) {
		/* The directory name contains metacharacters, so we
		   have to glob for the directory, and then glob for
		   the pattern in each directory found.  */
		glob_t dirs;
		register int i;
		status = glob(dirname,
			      ((flags &
				(GLOB_ERR | GLOB_NOCHECK | GLOB_NOESCAPE)) |
			       GLOB_NOSORT), errfunc, &dirs);
		if (status != 0)
			goto out;

		/* We have successfully globbed the preceding directory name.
		   For each name we found, call glob_in_dir on it and FILENAME,
		   appending the results to PGLOB.  */
		for (i = 0; i < dirs.gl_pathc; ++i) {
			int oldcount1;

#ifdef	SHELL
			{
				/* Make globbing interruptible in the bash shell. */
				extern int interrupt_state;

				if (interrupt_state) {
					globfree(&dirs);
					globfree(&files);
					status = GLOB_ABEND goto out;
				}
			}
#endif				/* SHELL.  */

			oldcount1 = pglob->gl_pathc;
			status = glob_in_dir(filename, dirs.gl_pathv[i],
					     (flags | GLOB_APPEND) &
					     ~GLOB_NOCHECK, errfunc, pglob);
			if (status == GLOB_NOMATCH) {
				/* No matches in this directory.  Try the next.  */
				continue;
			}

			if (status != 0) {
				globfree(pglob);
				goto out;
			}

			/* Stick the directory on the front of each name.  */
			prefix_array(dirs.gl_pathv[i],
					 &pglob->gl_pathv[oldcount1],
					 pglob->gl_pathc - oldcount1,
					 flags & GLOB_MARK);
		}

		globfree(&dirs);
		flags |= GLOB_MAGCHAR;

		if (pglob->gl_pathc == oldcount) {
			/* No matches.  */

			if (flags & GLOB_NOCHECK) {
				size_t len = strlen(pattern) + 1;
				char *patcopy = (char *)xmalloc(len);
				memcpy(patcopy, pattern, len);

				pglob->gl_pathv
				    = (char **)xrealloc(pglob->gl_pathv,
						       (pglob->gl_pathc +
							((flags & GLOB_DOOFFS) ?
							 pglob->gl_offs : 0) +
							1 + 1) *
						       sizeof(char *));

				if (flags & GLOB_DOOFFS)
					while (pglob->gl_pathc < pglob->gl_offs)
						pglob->gl_pathv[pglob->
								gl_pathc++] =
						    NULL;

				pglob->gl_pathv[pglob->gl_pathc++] = patcopy;
				pglob->gl_pathv[pglob->gl_pathc] = NULL;
				pglob->gl_flags = flags;
			} else {
				status = GLOB_NOMATCH;
				goto out;
			}
		}
	} else {
		status = glob_in_dir(filename, dirname, flags, errfunc, pglob);
		if (status != 0)
			goto out;

		if (dirlen > 0) {
			/* Stick the directory on the front of each name.  */
			prefix_array(dirname,
					 &pglob->gl_pathv[oldcount],
					 pglob->gl_pathc - oldcount,
					 flags & GLOB_MARK);
		}
	}

	if (flags & GLOB_MARK) {
		/* Append slashes to directory names.  glob_in_dir has already
		   allocated the extra character for us.  */
		int i;
		struct stat st;
		for (i = oldcount; i < pglob->gl_pathc; ++i)
			if (stat(pglob->gl_pathv[i], &st) == 0 &&
			    S_ISDIR(st.st_mode))
				strcat(pglob->gl_pathv[i], "/");
	}
#ifdef CONFIG_GLOB_SORT
	if (!(flags & GLOB_NOSORT))
		/* Sort the vector.  */
		qsort((__ptr_t) & pglob->gl_pathv[oldcount],
		      pglob->gl_pathc - oldcount,
		      sizeof(char *), collated_compare);
#endif
	status = 0;
out:
	free(dirname);

	return status;
}

/* Prepend DIRNAME to each of N members of ARRAY, replacing ARRAY's
   elements in place.  Return nonzero if out of memory, zero if successful.
   A slash is inserted between DIRNAME and each elt of ARRAY,
   unless DIRNAME is just "/".  Each old element of ARRAY is freed.
   If ADD_SLASH is non-zero, allocate one character more than
   necessary, so that a slash can be appended later.  */
static int prefix_array(const char *dirname, char **array, size_t n,
		int add_slash)
{
	register size_t i;
	size_t dirlen = strlen(dirname);

	if (dirlen == 1 && dirname[0] == '/')
		/* DIRNAME is just "/", so normal prepending would get us "//foo".
		   We want "/foo" instead, so don't prepend any chars from DIRNAME.  */
		dirlen = 0;

	for (i = 0; i < n; ++i) {
		size_t eltlen = strlen(array[i]) + 1;
		char *new =
		    (char *)xmalloc(dirlen + 1 + eltlen + (add_slash ? 1 : 0));

		memcpy(new, dirname, dirlen);
		new[dirlen] = '/';
		memcpy(&new[dirlen + 1], array[i], eltlen);
		free((__ptr_t) array[i]);
		array[i] = new;
	}

	return 0;
}

/* Like `glob', but PATTERN is a final pathname component,
   and matches are searched for in DIRECTORY.
   The GLOB_NOSORT bit in FLAGS is ignored.  No sorting is ever done.
   The GLOB_APPEND flag is assumed to be set (always appends).  */
static int glob_in_dir(const char *pattern, const char *directory,
		int flags, int (*errfunc) __P((const char *, int)), glob_t *pglob)
{
	__ptr_t stream = NULL;

	struct globlink {
		struct globlink *next;
		char *name;
	};
	struct globlink *names = NULL;
	size_t nfound = 0;
	int meta;

	meta = glob_pattern_p(pattern, !(flags & GLOB_NOESCAPE));

	if (meta)
		flags |= GLOB_MAGCHAR;

	if (meta)
		stream = opendir(directory);

	if (stream == NULL) {
		if ((errfunc != NULL && (*errfunc) (directory, errno)) ||
		    (flags & GLOB_ERR))
			return GLOB_ABORTED;
	}

	while (1) {
		const char *name;
		size_t len;

		struct dirent *d = readdir((DIR *) stream);
		if (d == NULL)
			break;
//		if (! (d->d_ino != 0))
//			continue;
		name = d->d_name;

		if ((!meta && strcmp(pattern, name) == 0)
		    || fnmatch(pattern, name,
			       (!(flags & GLOB_PERIOD) ? FNM_PERIOD : 0) |
			       ((flags & GLOB_NOESCAPE) ? FNM_NOESCAPE : 0)) == 0) {
			struct globlink *new =
			    (struct globlink *)xmalloc(sizeof(struct globlink));
			len = strlen(name);
			new->name = xmalloc(len + ((flags & GLOB_MARK) ? 1 : 0) + 1);
			memcpy((__ptr_t) new->name, name, len);
			new->name[len] = '\0';
			new->next = names;
			names = new;
			++nfound;
			if (!meta)
				break;
		}
	}

	if (nfound == 0 && (flags & GLOB_NOCHECK)) {
		size_t len = strlen(pattern);
		nfound = 1;
		names = (struct globlink *)xmalloc(sizeof(struct globlink));
		names->next = NULL;
		names->name =
		    (char *)xmalloc(len + (flags & GLOB_MARK ? 1 : 0) + 1);
		memcpy(names->name, pattern, len);
		names->name[len] = '\0';
	}

	pglob->gl_pathv
	    = (char **)xrealloc(pglob->gl_pathv,
			       (pglob->gl_pathc +
				((flags & GLOB_DOOFFS) ? pglob->gl_offs : 0) +
				nfound + 1) * sizeof(char *));

	if (flags & GLOB_DOOFFS)
		while (pglob->gl_pathc < pglob->gl_offs)
			pglob->gl_pathv[pglob->gl_pathc++] = NULL;

	while (names) {
		struct globlink *tmp;

		pglob->gl_pathv[pglob->gl_pathc++] = names->name;
		tmp = names;
		names = names->next;
		free(tmp);
	}
	pglob->gl_pathv[pglob->gl_pathc] = NULL;

	pglob->gl_flags = flags;

	{
		int save = errno;
		(void)closedir((DIR *) stream);
		errno = save;
	}
	return nfound == 0 ? GLOB_NOMATCH : 0;
}
#endif /* CONFIG_GLOB */

#ifdef CONFIG_FAKE_GLOB
/* Fake version of glob. We simply put the input string into
 * the gl_pathv array. Currently we don't need it as hush.c won't
 * call us if no glob support is available.
 */
int glob(pattern, flags, errfunc, pglob)
const char *pattern;
int flags;
int (*errfunc) __P((const char *, int));
glob_t *pglob;
{
	int elems, i;

	if (!(flags & GLOB_APPEND)) {
		pglob->gl_pathc = 0;
		pglob->gl_pathv = NULL;
	}

	elems = pglob->gl_pathc + 2;
	if (flags & GLOB_DOOFFS)
		elems += pglob->gl_offs;

	pglob->gl_pathv = xrealloc(pglob->gl_pathv, elems * sizeof(char *));

	if (flags & GLOB_DOOFFS)
		for (i = 0; i < pglob->gl_offs; i++)
			pglob->gl_pathv[i] = NULL;

	pglob->gl_pathv[pglob->gl_pathc] = strdup(pattern);
	pglob->gl_pathc++;
	pglob->gl_pathv[pglob->gl_pathc] = NULL;

	return 0;
}
#endif /* CONFIG_FAKE_GLOB */

/* Free storage allocated in PGLOB by a previous `glob' call.  */
void globfree(glob_t *pglob)
{
	if (pglob->gl_pathv != NULL) {
		int i = pglob->gl_flags & GLOB_DOOFFS ? pglob->gl_offs : 0;
		for (; i < pglob->gl_pathc; ++i)
			if (pglob->gl_pathv[i] != NULL)
				free((__ptr_t) pglob->gl_pathv[i]);
		free((__ptr_t) pglob->gl_pathv);
	}
}
