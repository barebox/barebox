/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _EFI_ATTRIBUTES_H_
#define _EFI_ATTRIBUTES_H_

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

#endif
