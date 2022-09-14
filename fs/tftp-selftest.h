// SPDX-License-Identifier: GPL-2.0+
// SPDX-FileCopyrightText: 2022 Enrico Scholz <enrico.scholz@sigma-chemnitz.de>

#ifndef H_BAREBOX_FS_TFTP_SELFTEST_H
#define H_BAREBOX_FS_TFTP_SELFTEST_H

#include <bselftest.h>

#define _expect_fmt(_v) _Generic(_v,				\
				 void const *: "%p",		\
				 void *: "%p",			\
				 bool: "%d",			\
				 long int: "%lld",		\
				 long unsigned int: "%llu",	\
				 unsigned short: "%h",		\
				 signed int: "%d",		\
				 unsigned int: "%u")

#define _expect_op(_a, _b, _op)						\
	({								\
		__typeof__(_a)	__a = (_a);				\
		__typeof__(_b)	__b = (_b);				\
									\
		total_tests++;						\
									\
		if (!(__a _op __b)) {					\
			char fmt[sizeof "%s:%d: failed: %XXX  %XXX\n" # _op]; \
			strcpy(fmt, "%s:%d: failed: ");			\
			strcat(fmt, _expect_fmt(__a));			\
			strcat(fmt, " " # _op " ");			\
			strcat(fmt, _expect_fmt(__b));			\
			strcat(fmt, "\n");				\
									\
			failed_tests++;					\
			printf(fmt, __func__, __LINE__, __a, __b);	\
		}							\
	})

#define _as_void(_p) ({					\
			void const *__res = (_p);	\
			__res;				\
		})

#define expect_eq(_a, _b)	_expect_op(_a, _b, ==);
#define expect_ne(_a, _b)	_expect_op(_a, _b, !=);
#define expect_it(_a)		_expect_op(_a, true, ==);

#define expect_err(_a)		_expect_op(_a, 0, <);
#define expect_ok(_a)		_expect_op(_a, 0, ==);

/* _Generic() does not match 'void *' for typed pointers; cast them to raw
   'void *' first */
#define expect_NULL(_a)		_expect_op(_as_void(_a), NULL, ==);
#define expect_PTR(_a)		_expect_op(_as_void(_a), NULL, !=);

#endif	/* H_BAREBOX_FS_TFTP_SELFTEST_H */
