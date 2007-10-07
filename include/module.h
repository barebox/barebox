#ifndef __MODULE_H
#define __MODULE_H

#include <elf.h>
#include <asm/module.h>
#include <compiler.h>

#ifndef MODULE_SYMBOL_PREFIX
#define MODULE_SYMBOL_PREFIX
#endif

#ifdef CONFIG_MODULES
struct kernel_symbol
{
	unsigned long value;
	const char *name;
};

/* For every exported symbol, place a struct in the __ksymtab section */
#define __EXPORT_SYMBOL(sym, sec)				\
	extern typeof(sym) sym;					\
	static const char __ustrtab_##sym[]			\
	__attribute__((section("__usymtab_strings")))		\
	= MODULE_SYMBOL_PREFIX #sym;                    	\
	static const struct kernel_symbol __usymtab_##sym	\
	__used \
	__attribute__((section("__usymtab" sec), unused))	\
	= { (unsigned long)&sym, __ustrtab_##sym }

#define EXPORT_SYMBOL(sym)					\
	__EXPORT_SYMBOL(sym, "")
#else
#define EXPORT_SYMBOL(sym)
#endif

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

