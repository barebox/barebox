#ifndef __MODULE_H
#define __MODULE_H

#include <elf.h>
#include <asm/module.h>

struct module {
	char *name;

	/* Startup function. */
	int (*init)(void);

	/* Here is the actual code + data, free'd on unload. */
	void *module_core;

	/* Arch-specific module values */
	struct mod_arch_specific arch;

	unsigned long core_size;
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

#endif /* __MODULE_H */

