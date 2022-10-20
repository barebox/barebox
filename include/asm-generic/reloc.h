/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _ASM_GENERIC_RELOC_H_
#define _ASM_GENERIC_RELOC_H_

#include <linux/build_bug.h>
#include <linux/compiler.h>

#ifndef global_variable_offset
#define global_variable_offset() get_runtime_offset()
#endif

/*
 * Using sizeof() on incomplete types always fails, so we use GCC's
 * __builtin_object_size() instead. This is the mechanism underlying
 * FORTIFY_SOURCE. &symbol should always be something GCC can compute
 * a size for, even without annotations, unless it's incomplete.
 * The second argument ensures we get 0 for failure.
 */
#define __has_type_complete(sym) __builtin_object_size(&(sym), 2)

#define __has_type_byte_array(sym) (sizeof(*sym) == 1 + __must_be_array(sym))

/*
 * runtime_address() defined below is supposed to be used exclusively
 * with linker defined symbols, e.g. unsigned char input_end[].
 *
 * We can't completely ensure that, but this gets us close enough
 * to avoid most abuse of runtime_address().
 */
#define __is_incomplete_byte_array(sym) \
	(!__has_type_complete(sym) && __has_type_byte_array(sym))

/*
 * While accessing global variables before C environment is setup is
 * questionable, we can't avoid it when we decide to write our
 * relocation routines in C. This invites a tricky problem with
 * this naive code:
 *
 *   var = &variable + global_variable_offset(); relocate_to_current_adr();
 *
 * Compiler is within rights to rematerialize &variable after
 * relocate_to_current_adr(), which is unfortunate because we
 * then end up adding a relocated &variable with the relocation
 * offset once more. We avoid this here by hiding address with
 * RELOC_HIDE. This is required as a simple compiler barrier()
 * with "memory" clobber is not immune to compiler proving that
 * &sym fits in a register and as such is unaffected by the memory
 * clobber. barrier_data(&sym) would work too, but that comes with
 * aforementioned compiler "memory" barrier, that we don't care for.
 *
 * We don't necessarily need the volatile variable assignment when
 * using the compiler-gcc.h RELOC_HIDE implementation as __asm__
 * __volatile__ takes care of it, but the generic RELOC_HIDE
 * implementation has GCC misscompile runtime_address() when not passing
 * in a volatile object. Volatile casts instead of variable assignments
 * also led to miscompilations with GCC v11.1.1 for THUMB2.
 */

#define runtime_address(sym) ({					\
	void *volatile __addrof_sym = (sym);			\
	if (!__is_incomplete_byte_array(sym))			\
		__unsafe_runtime_address();			\
	RELOC_HIDE(__addrof_sym, global_variable_offset());	\
})

/*
 * Above will fail for "near" objects, e.g. data in the same
 * translation unit or with LTO, as the compiler can be smart
 * enough to omit relocation entry and just generate PC relative
 * accesses leading to base address being added twice. We try to
 * catch most of these here by triggering an error when runtime_address()
 * is used with anything that is not a byte array of unknown size.
 */
extern void *__compiletime_error(
	"runtime_address() may only be called on linker defined symbols."
) __unsafe_runtime_address(void);

#endif
