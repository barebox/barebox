/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _ASM_RELOC_H_
#define _ASM_RELOC_H_

#include <asm/sections.h>

unsigned long get_runtime_offset(void);

/* global_variable_offset() - Access global variables when not running at link address
 *
 * Get the offset of global variables when not running at the address we are
 * linked at.
 */
static inline __prereloc unsigned long global_variable_offset(void)
{
#ifdef CONFIG_CPU_V8
	unsigned long text;

	__asm__ __volatile__(
		"adrp   %0, _text\n"
		"add    %0, %0, :lo12:_text\n"
		: "=r" (text)
		:
		: "memory");
	return text - (unsigned long)_text;
#else
	return get_runtime_offset();
#endif
}
#define global_variable_offset() global_variable_offset()

void relocate_image(unsigned long offset,
		    void *dstart, void *dend,
		    long *dynsym, long *dynend);
void relocate_to_current_adr(void);
void relocate_to_adr(unsigned long target);
void relocate_to_adr_full(unsigned long target);

void pbl_barebox_break(void);

void setup_c(void);

#include <asm-generic/reloc.h>

#endif
