/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _ASM_GENERIC_SECTIONS_H_
#define _ASM_GENERIC_SECTIONS_H_

#include <linux/types.h>

extern char _text[], _stext[], _etext[];
extern char __start_rodata[], __end_rodata[];
extern char __bss_start[], __bss_stop[];
extern char _sdata[], _edata[];
extern char __bare_init_start[], __bare_init_end[];
extern char _end[];
extern char __image_start[];
extern char __image_end[];
extern char __piggydata_start[];
extern void *_barebox_image_size;
extern void *_barebox_bare_init_size;
extern void *_barebox_pbl_size;

/* Start and end of .ctors section - used for constructor calls. */
extern char __ctors_start[], __ctors_end[];

#define barebox_image_size	(__image_end - __image_start)
#define barebox_bare_init_size	(unsigned int)&_barebox_bare_init_size
#define barebox_pbl_size	(__piggydata_start - __image_start)

/**
 * is_barebox_rodata - checks if the pointer address is located in the
 *                    .rodata section
 *
 * @addr: address to check
 *
 * Returns: true if the address is located in .rodata, false otherwise.
 */
static inline bool is_barebox_rodata(unsigned long addr)
{
	return addr >= (unsigned long)__start_rodata &&
	       addr < (unsigned long)__end_rodata;
}

#endif /* _ASM_GENERIC_SECTIONS_H_ */
