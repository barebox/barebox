/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _ASM_GENERIC_BUG_H
#define _ASM_GENERIC_BUG_H

#include <linux/compiler.h>
#include <printk.h>

#define BUG() do { \
	printf("BUG: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __FUNCTION__); \
	panic("BUG!"); \
} while (0)
#define BUG_ON(condition) do { if (unlikely((condition)!=0)) BUG(); } while(0)


#define __WARN() do { 								\
	printf("WARNING: at %s:%d/%s()!\n", __FILE__, __LINE__, __FUNCTION__);	\
} while (0)

#ifndef WARN_ON
#define WARN_ON(condition) ({						\
	int __ret_warn_on = !!(condition);				\
	if (unlikely(__ret_warn_on))					\
		__WARN();						\
	unlikely(__ret_warn_on);					\
})
#endif

#ifndef WARN
#define WARN(condition, format...) ({					\
	int __ret_warn_on = !!(condition);				\
	if (unlikely(__ret_warn_on)) {					\
		__WARN();						\
		printf("WARNING: " format);				\
	}								\
	unlikely(__ret_warn_on);					\
})
#endif

#define WARN_ONCE(condition, format...) ({	\
	static int __warned;			\
	int __ret_warn_once = !!(condition);	\
						\
	if (unlikely(__ret_warn_once)) {	\
		if (WARN(!__warned, format)) {	\
			__warned = 1;		\
			dump_stack();		\
		}				\
	}					\
	unlikely(__ret_warn_once);		\
})

#define WARN_ON_ONCE(condition) ({			\
	static int __warned;				\
	int __ret_warn_once = !!(condition);		\
							\
	if (unlikely(__ret_warn_once && !__warned)) {	\
		__warned = 1;				\
		__WARN();				\
	}						\
	unlikely(__ret_warn_once);			\
})

#define ASSERT(expr)  do {				\
	if (IS_ENABLED(CONFIG_BUG_ON_DATA_CORRUPTION))	\
		BUG_ON(!(expr));			\
} while (0)

#endif
