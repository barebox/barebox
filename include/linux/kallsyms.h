/* SPDX-License-Identifier: GPL-2.0 */
/* SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/include/linux/kallsyms.h?id=8d3ae59288f1e7d58d76558a6ee96d533bc5019f */
/* Rewritten and vastly simplified by Rusty Russell for in-kernel
 * module loader:
 *   Copyright 2002 Rusty Russell <rusty@rustcorp.com.au> IBM Corporation
 */
#ifndef _LINUX_KALLSYMS_H
#define _LINUX_KALLSYMS_H

#include <linux/errno.h>
#include <linux/types.h>
#include <linux/stddef.h>
#include <module.h>

#include <asm/sections.h>

#define KSYM_NAME_LEN 512
#define KSYM_SYMBOL_LEN (sizeof("%s+%#lx/%#lx [%s]") + \
			(KSYM_NAME_LEN - 1) + \
			2*(BITS_PER_LONG*3/10) + (MODULE_NAME_LEN - 1) + 1)

/* barebox always generates kallsyms with --all-symbols */
static inline int is_ksym_addr(unsigned long addr)
{
	return addr >= (unsigned long)_stext && addr < (unsigned long)_end;
}

#ifdef CONFIG_KALLSYMS
unsigned long kallsyms_sym_address(int idx);

/* Lookup the address for a symbol. Returns 0 if not found. */
unsigned long kallsyms_lookup_name(const char *name);

extern int kallsyms_lookup_size_offset(unsigned long addr,
				  unsigned long *symbolsize,
				  unsigned long *offset);

/* Lookup an address.  modname is set to NULL if it's in barebox itself. */
const char *kallsyms_lookup(unsigned long addr,
			    unsigned long *symbolsize,
			    unsigned long *offset,
			    char **modname, char *namebuf);

/* Look up a symbol and return it in a text buffer. */
extern int sprint_symbol(char *buffer, unsigned long address);
extern int sprint_symbol_no_offset(char *buffer, unsigned long address);

#else /* !CONFIG_KALLSYMS */

static inline unsigned long kallsyms_lookup_name(const char *name)
{
	return 0;
}

static inline int kallsyms_lookup_size_offset(unsigned long addr,
					      unsigned long *symbolsize,
					      unsigned long *offset)
{
	return 0;
}

static inline const char *kallsyms_lookup(unsigned long addr,
					  unsigned long *symbolsize,
					  unsigned long *offset,
					  char **modname, char *namebuf)
{
	return NULL;
}

static inline int sprint_symbol(char *buffer, unsigned long addr)
{
	*buffer = '\0';
	return 0;
}

static inline int sprint_symbol_no_offset(char *buffer, unsigned long addr)
{
	*buffer = '\0';
	return 0;
}

#endif /*CONFIG_KALLSYMS*/

#endif /*_LINUX_KALLSYMS_H*/
