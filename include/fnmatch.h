/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Copyright (C) 1991,92,93,96,97,98,99,2001 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
*/

#ifndef	_FNMATCH_H
#define	_FNMATCH_H

/* Bits set in the FLAGS argument to `fnmatch'.  */
#define	FNM_PATHNAME	(1 << 0) /* No wildcard can ever match `/'.  */
#define	FNM_NOESCAPE	(1 << 1) /* Backslashes don't quote special chars.  */
#define	FNM_PERIOD	(1 << 2) /* Leading `.' is matched only explicitly.  */
#define FNM_FILE_NAME	 FNM_PATHNAME	/* Preferred GNU name.  */
#define FNM_LEADING_DIR	(1 << 3)	/* Ignore `/...' after a match.  */
#define FNM_CASEFOLD	(1 << 4)	/* Compare without regard to case.  */

/* Value returned by `fnmatch' if STRING does not match PATTERN.  */
#define	FNM_NOMATCH	1

/* Match NAME against the filename pattern PATTERN,
   returning zero if it matches, FNM_NOMATCH if not.  */
extern int fnmatch(const char *pattern, const char *name, int flags);

#endif /* fnmatch.h */
