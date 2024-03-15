/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _LINUX_EXPORT_H
#define _LINUX_EXPORT_H

#ifndef __ASSEMBLY__

#define THIS_MODULE	0

#if defined(CONFIG_MODULES) && !defined(__DISABLE_EXPORTS)

struct kernel_symbol
{
	unsigned long value;
	const char *name;
};

/* For every exported symbol, place a struct in the __ksymtab section */
#define __EXPORT_SYMBOL(sym)					\
	extern typeof(sym) sym;					\
	static const char __ustrtab_##sym[]			\
	__ll_elem(__usymtab_strings)				\
	= MODULE_SYMBOL_PREFIX #sym;                    	\
	static const struct kernel_symbol __usymtab_##sym	\
	__ll_elem(__usymtab)					\
	= { (unsigned long)&sym, __ustrtab_##sym }

#define EXPORT_SYMBOL(sym)					\
	__EXPORT_SYMBOL(sym)

#define EXPORT_SYMBOL_GPL(sym)					\
	__EXPORT_SYMBOL(sym)

#else

#define EXPORT_SYMBOL(sym)
#define EXPORT_SYMBOL_GPL(sym)

#endif /* CONFIG_MODULES */

#endif /* __ASSEMBLY__ */

#endif /* _LINUX_EXPORT_H */
