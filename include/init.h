/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _INIT_H
#define _INIT_H

#include <linux/compiler_types.h>

/*
 * fake define to simplify the linux sync
 */
#define __init
#define __initdata
#define __initconst
#define __exit
#define __exitdata

/* For assembly routines */
#define __BARE_INIT	.section ".text_bare_init.text","ax"

#ifndef __ASSEMBLY__
typedef int (*initcall_t)(void);
typedef void (*exitcall_t)(void);

/* section for code used very early when
 * - we're not running from where we linked at
 * - bss not cleared
 * - static variables not initialized
 *
 * Mainly useful for booting from NAND Controllers
 */
#define __bare_init          __section(.text_bare_init.text)

#endif

#ifndef MODULE

#ifndef __ASSEMBLY__

#define __define_initcall(fn,id) \
	static initcall_t __initcall_##fn##id __ll_elem(.initcall.##id) = fn

#define __define_exitcall(fn,id) \
	static exitcall_t __exitcall_##fn##id __ll_elem(.exitcall.##id) = fn


/*
 * A "pure" initcall has no dependencies on anything else, and purely
 * initializes variables that couldn't be statically initialized.
 *
 * This only exists for built-in code, not for modules.
 *
 * The only purpose for "of_populate" is to call of_probe() other functions are
 * not allowed.
 */
#define pure_initcall(fn)		__define_initcall(fn, 0)

#define core_initcall(fn)		__define_initcall(fn, 1)
#define postcore_initcall(fn)		__define_initcall(fn, 2)
#define console_initcall(fn)		__define_initcall(fn, 3)
#define postconsole_initcall(fn)	__define_initcall(fn, 4)
#define mem_initcall(fn)		__define_initcall(fn, 5)
#define postmem_initcall(fn)		__define_initcall(fn, 6)
#define mmu_initcall(fn)		__define_initcall(fn, 7)
#define postmmu_initcall(fn)		__define_initcall(fn, 8)
#define coredevice_initcall(fn)		__define_initcall(fn, 9)
#define fs_initcall(fn)			__define_initcall(fn, 10)
#define device_initcall(fn)		__define_initcall(fn, 11)
#define crypto_initcall(fn)		__define_initcall(fn, 12)
#define of_populate_initcall(fn)	__define_initcall(fn, 13)
#define late_initcall(fn)		__define_initcall(fn, 14)
#define environment_initcall(fn)	__define_initcall(fn, 15)
#define postenvironment_initcall(fn)	__define_initcall(fn, 16)

#define early_exitcall(fn)		__define_exitcall(fn, 0)
#define predevshutdown_exitcall(fn)	__define_exitcall(fn, 1)
#define devshutdown_exitcall(fn)	__define_exitcall(fn, 2)
#define postdevshutdown_exitcall(fn)	__define_exitcall(fn, 3)
#define prearchshutdown_exitcall(fn)	__define_exitcall(fn, 4)
#define archshutdown_exitcall(fn)	__define_exitcall(fn, 5)
#define postarchshutdown_exitcall(fn)	__define_exitcall(fn, 6)

#endif /* __ASSEMBLY__ */

#endif /* MODULE */

#endif /* _INIT_H */

