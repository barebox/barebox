/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Copyright (C) 1991,92,95,96,97,98,2000,2001 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
*/

#ifndef	_GLOB_H
#define	_GLOB_H

#include <linux/types.h>

/* Bits set in the FLAGS argument to `glob'.  */
#define	GLOB_ERR	(1 << 0)/* Return on read errors.  */
#define	GLOB_MARK	(1 << 1)/* Append a slash to each name.  */
#define	GLOB_NOSORT	(1 << 2)/* Don't sort the names.  */
#define	GLOB_DOOFFS	(1 << 3)/* Insert PGLOB->gl_offs NULLs.  */
#define	GLOB_NOCHECK	(1 << 4)/* If nothing matches, return the pattern.  */
#define	GLOB_APPEND	(1 << 5)/* Append to results of a previous call.  */
#define	GLOB_NOESCAPE	(1 << 6)/* Backslashes don't quote metacharacters.  */
#define	GLOB_PERIOD	(1 << 7)/* Leading `.' can be matched by metachars.  */
#define	GLOB_MAGCHAR	 (1 << 8)/* Set in gl_flags if any metachars seen.  */
# define __GLOB_FLAGS	(GLOB_ERR|GLOB_MARK|GLOB_NOSORT|GLOB_DOOFFS| \
			 GLOB_NOESCAPE|GLOB_NOCHECK|GLOB_APPEND|     \
			 GLOB_PERIOD)

/* Error returns from `glob'.  */
#define	GLOB_NOSPACE	1	/* Ran out of memory.  */
#define	GLOB_ABORTED	2	/* Read error.  */
#define	GLOB_NOMATCH	3	/* No matches found.  */
#define GLOB_NOSYS	4	/* Not implemented.  */
#define GLOB_ABEND	GLOB_ABORTED

typedef struct {
	size_t gl_pathc;		/* Count of paths matched by the pattern.  */
	char **gl_pathv;		/* List of matched pathnames.  */
	size_t gl_offs;		/* Slots to reserve in `gl_pathv'.  */
	int gl_flags;		/* Set to FLAGS, maybe | GLOB_MAGCHAR.  */
} glob_t;

#ifdef CONFIG_GLOB
extern int glob (const char *__restrict pattern, int flags,
		      int (*errfunc) (const char *, int),
		      glob_t *__restrict pglob);

extern void globfree(glob_t *pglob);
#else
static inline int glob(const char *__restrict pattern, int flags,
		      int (*errfunc) (const char *, int),
		      glob_t *__restrict pglob)
{
	return GLOB_ABORTED;
}

static inline void globfree(glob_t *pglob)
{
}
#endif

/* Return nonzero if PATTERN contains any metacharacters.
   Metacharacters can be quoted with backslashes if QUOTE is nonzero.

   This function is not part of the interface specified by POSIX.2
   but several programs want to use it.  */
extern int glob_pattern_p(const char *pattern, int quote);

#endif /* glob.h  */
