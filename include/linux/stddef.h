#ifndef _LINUX_STDDEF_H
#define _LINUX_STDDEF_H

#undef NULL
#if defined(__cplusplus)
#define NULL 0
#else
#define NULL ((void *)0)
#endif

enum {
	false	= 0,
	true	= 1
};

#ifndef _SIZE_T
#include <linux/types.h>
#endif

typedef unsigned short wchar_t;

#undef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#endif
