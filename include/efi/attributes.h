/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _EFI_ATTRIBUTES_H_
#define _EFI_ATTRIBUTES_H_

#ifndef __ASSEMBLY__

#include <linux/compiler.h>

#ifdef __x86_64__
#define EFIAPI __attribute__((ms_abi))
#define efi_va_list __builtin_ms_va_list
#define efi_va_start __builtin_ms_va_start
#define efi_va_arg __builtin_va_arg
#define efi_va_copy __builtin_ms_va_copy
#define efi_va_end __builtin_ms_va_end
#else
#define EFIAPI
#define efi_va_list va_list
#define efi_va_start va_start
#define efi_va_arg va_arg
#define efi_va_copy va_copy
#define efi_va_end va_end
#endif /* __x86_64__ */

#ifdef CONFIG_EFI_RUNTIME
/**
 * __efi_runtime_data - declares a non-const variable for EFI runtime section
 *
 * This macro indicates that a variable is non-const and should go into the
 * EFI runtime section, and thus still be available when the OS is running.
 *
 * Only use on variables not declared const.
 *
 * Example:
 *
 * ::
 *
 *   static __efi_runtime_data my_computed_table[256];
 */
#ifndef __efi_runtime_data
#define __efi_runtime_data __section(.efi_runtime.data)
#endif

/**
 * __efi_runtime_rodata - declares a read-only variable for EFI runtime section
 *
 * This macro indicates that a variable is read-only (const) and should go into
 * the EFI runtime section, and thus still be available when the OS is running.
 *
 * Only use on variables also declared const.
 *
 * Example:
 *
 * ::
 *
 *   static const __efi_runtime_rodata my_const_table[] = { 1, 2, 3 };
 */
#ifndef __efi_runtime_rodata
#define __efi_runtime_rodata __section(.efi_runtime.rodata)
#endif

/**
 * __efi_runtime - declares a function for EFI runtime section
 *
 * This macro indicates that a function should go into the EFI runtime section,
 * and thus still be available when the OS is running.
 *
 * Example:
 *
 * ::
 *
 *   static __efi_runtime compute_my_table(void);
 */
#ifndef __efi_runtime
#define __efi_runtime __section(.efi_runtime.text) \
	notrace __no_sanitize_address __no_stack_protector
#endif
#endif /* CONFIG_EFI_RUNTIME */

/* We #ifndef beforehand to allow compiler flags to override */
#ifndef __efi_runtime_data
#define __efi_runtime_data
#endif

#ifndef __efi_runtime_rodata
#define __efi_runtime_rodata
#endif

#ifndef __efi_runtime
#define __efi_runtime
#endif

#else  /* __ASSEMBLY__ */

#if defined(CONFIG_EFI_RUNTIME) && !defined(EFI_RUNTIME_SECTION)
#define EFI_RUNTIME_SECTION(sect) .efi_runtime##sect
#endif

#ifndef EFI_RUNTIME_SECTION
#define EFI_RUNTIME_SECTION(sect) sect
#endif

#endif

#endif
