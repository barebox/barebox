/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __MODULE_H
#define __MODULE_H

#include <elf.h>
#include <linux/compiler.h>
#include <linux/export.h>
#include <linux/list.h>

#ifndef MODULE_SYMBOL_PREFIX
#define MODULE_SYMBOL_PREFIX
#endif

#define MODULE_NAME_LEN (64 - sizeof(unsigned long))

#ifdef CONFIG_MODULES
#include <asm/module.h>

struct module * load_module(void *mod_image, unsigned long len);

struct module {
	/* Unique handle for this module */
	char name[MODULE_NAME_LEN];

	/* Startup function. */
	int (*init)(void);

	/* Here is the actual code + data, free'd on unload. */
	void *module_core;

	/* Arch-specific module values */
	struct mod_arch_specific arch;

	unsigned long core_size;

	struct list_head list;
};

/* Apply the given relocation to the (simplified) ELF.  Return -error
   or 0. */
int apply_relocate(Elf_Shdr *sechdrs,
		   const char *strtab,
		   unsigned int symindex,
		   unsigned int relsec,
		   struct module *mod);

/* Apply the given add relocation to the (simplified) ELF.  Return
   -error or 0 */
int apply_relocate_add(Elf_Shdr *sechdrs,
		       const char *strtab,
		       unsigned int symindex,
		       unsigned int relsec,
		       struct module *mod);
#endif /* CONFIG_MODULES */

extern struct list_head module_list;

#define for_each_module(m) \
	list_for_each_entry(m, &module_list, list)

#endif /* __MODULE_H */

