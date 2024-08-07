/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __MODULE_H
#define __MODULE_H

#include <init.h>
#include <elf.h>
#include <linux/compiler.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/list.h>

#define MODULE_NAME_LEN (64 - sizeof(unsigned long))

/* These are either module local, or the kernel's dummy ones. */
extern int init_module(void);
extern void cleanup_module(void);

#ifndef MODULE
/**
 * module_init() - driver initialization entry point
 * @x: function to be run at kernel boot time or module insertion
 *
 * module_init() will either be called during do_initcalls() (if
 * builtin) or at module insertion time (if a module).  There can only
 * be one per module.
 */
#define module_init(x)	device_initcall(x);

/**
 * module_exit() - driver exit entry point
 * @x: function to be run when driver is removed
 *
 * module_exit() will wrap the driver clean-up code
 * with cleanup_module() when used with rmmod when
 * the driver is a module.  If the driver is statically
 * compiled into the kernel, module_exit() has no effect.
 * There can only be one per module.
 */
#define module_exit(x)	devshutdown_exitcall(x);

#else /* MODULE */

/*
 * In most cases loadable modules do not need custom
 * initcall levels. There are still some valid cases where
 * a driver may be needed early if built in, and does not
 * matter when built as a loadable module. Like bus
 * snooping debug drivers.
 */
#define core_initcall(fn)		module_init(fn)
#define postcore_initcall(fn)		module_init(fn)
#define console_initcall(fn)		module_init(fn)
#define postconsole_initcall(fn)	module_init(fn)
#define mem_initcall(fn)		module_init(fn)
#define mmu_initcall(fn)		module_init(fn)
#define postmmu_initcall(fn)		module_init(fn)
#define coredevice_initcall(fn)		module_init(fn)
#define fs_initcall(fn)			module_init(fn)
#define device_initcall(fn)		module_init(fn)
#define late_initcall(fn)		module_init(fn)

#define early_exitcall(fn)		module_exit(fn)
#define predevshutdown_exitcall(fn)	module_exit(fn)
#define devshutdown_exitcall(fn)	module_exit(fn)
#define postdevshutdown_exitcall(fn)	module_exit(fn)
#define prearchshutdown_exitcall(fn)	module_exit(fn)
#define archshutdown_exitcall(fn)	module_exit(fn)
#define postarchshutdown_exitcall(fn)	module_exit(fn)

/* Each module must use one module_init(). */
#define module_init(initfn)					\
	static inline initcall_t __maybe_unused __inittest(void)		\
	{ return initfn; }					\
	int init_module(void) __attribute__((alias(#initfn)));

/* This is only required if you want to be unloadable. */
#define module_exit(exitfn)					\
	static inline exitcall_t __maybe_unused __exittest(void)		\
	{ return exitfn; }					\
	void cleanup_module(void) __attribute__((alias(#exitfn)));

#endif

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

/* Adjust arch-specific sections.  Return 0 on success.  */
int module_frob_arch_sections(Elf_Ehdr *hdr,
			      Elf_Shdr *sechdrs,
			      char *secstrings,
			      struct module *mod);

#endif /* CONFIG_MODULES */

extern struct list_head module_list;

#define for_each_module(m) \
	list_for_each_entry(m, &module_list, list)

#endif /* __MODULE_H */

