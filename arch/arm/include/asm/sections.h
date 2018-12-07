#ifndef __ASM_SECTIONS_H
#define __ASM_SECTIONS_H

#ifndef __ASSEMBLY__
#include <asm-generic/sections.h>
#include <linux/types.h>

extern char __rel_dyn_start[];
extern char __rel_dyn_end[];
extern char __dynsym_start[];
extern char __dynsym_end[];
extern char __exceptions_start[];
extern char __exceptions_stop[];

#endif

#endif /* __ASM_SECTIONS_H */
