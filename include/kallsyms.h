/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __KALLSYMS_H
#define __KALLSYMS_H

#define KSYM_NAME_LEN 128
#define KSYM_SYMBOL_LEN (sizeof("%s+%#lx/%#lx [%s]") + (KSYM_NAME_LEN - 1) + \
		2*(BITS_PER_LONG*3/10) + (MODULE_NAME_LEN - 1) + 1)
unsigned long kallsyms_lookup_name(const char *name);
const char *kallsyms_lookup(unsigned long addr,
			    unsigned long *symbolsize,
			    unsigned long *offset,
			    char **modname, char *namebuf);

/* Look up a kernel symbol and return it in a text buffer. */
int sprint_symbol(char *buffer, unsigned long address);

#endif /* __KALLSYMS_H */
