/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __EFI_LOADER_TRACE_H__
#define __EFI_LOADER_TRACE_H__

#include <efi/error.h>
#include <linux/printk.h>

#ifndef __EFI_PRINT
#ifdef CONFIG_DEBUG_EFI_LOADER_ENTRY
#define __EFI_PRINT(...)	pr_info(__VA_ARGS__)
#define __EFI_WARN(...)		pr_warning(__VA_ARGS__)
#else
#define __EFI_PRINT(...)	pr_debug(__VA_ARGS__)
#define __EFI_WARN(...)		pr_debug(__VA_ARGS__)
#endif
#endif

const char *__efi_nesting(void);
const char *__efi_nesting_inc(void);
const char *__efi_nesting_dec(void);

/*
 * Enter the barebox world from UEFI:
 */
#ifndef EFI_ENTRY
#define EFI_ENTRY(format, ...) do { \
	__EFI_PRINT("%sEFI: Entry %s(" format ")\n", __efi_nesting_inc(), \
		__func__, ##__VA_ARGS__); \
	} while(0)
#endif

/*
 * Exit the barebox world back to UEFI:
 */
#ifndef EFI_EXIT
#define EFI_EXIT(ret) ({ \
	typeof(ret) _r = ret; \
	__EFI_PRINT("%sEFI: Exit: %s: %s (%u)\n", __efi_nesting_dec(), \
		__func__, efi_strerror((uintptr_t)_r), (u32)((uintptr_t) _r & ~EFI_ERROR_MASK)); \
	_r; \
	})
#endif

#ifndef EFI_EXIT2
#define EFI_EXIT2(ret, val) ({ \
	typeof(ret) _r = ret; \
	__EFI_PRINT("%sEFI: Exit: %s: %s (%u) = 0x%llx\n", __efi_nesting_dec(), \
		__func__, efi_strerror((uintptr_t)_r), (u32)((uintptr_t) _r & ~EFI_ERROR_MASK), \
		(u64)(uintptr_t)(val)); \
	_r; \
	})
#endif

/*
 * Call non-void UEFI function from barebox and retrieve return value:
 */
#ifndef EFI_CALL
#define EFI_CALL(exp) ({ \
	__EFI_PRINT("%sEFI: Call: %s\n", __efi_nesting_inc(), #exp); \
	typeof(exp) _r = exp; \
	__EFI_PRINT("%sEFI: %lu returned by %s\n", __efi_nesting_dec(), \
	      (unsigned long)((uintptr_t)_r & ~EFI_ERROR_MASK), #exp); \
	_r; \
})
#endif

/**
 * define EFI_RETURN() - return from EFI_CALL in efi_start_image()
 *
 * @ret:	status code
 */
#ifndef EFI_RETURN
#define EFI_RETURN(ret) ({ \
	typeof(ret) _r = ret; \
	__EFI_PRINT("%sEFI: %lu returned by started image", __efi_nesting_dec(), \
		    (unsigned long)((uintptr_t)_r & ~EFI_ERROR_MASK)); \
})
#endif

/*
 * Call void UEFI function from barebox:
 */
#ifndef EFI_CALL_VOID
#define EFI_CALL_VOID(exp) do { \
	__EFI_PRINT("%sEFI: Call: %s\n", __efi_nesting_inc(), #exp); \
	exp; \
	__EFI_PRINT("%sEFI: Return From: %s\n", __efi_nesting_dec(), #exp); \
	} while(0)
#endif

/*
 * Write an indented message with EFI prefix
 */
#ifndef EFI_PRINT
#define EFI_PRINT(format, ...) ({ \
	__EFI_PRINT("%sEFI: " format, __efi_nesting(), \
		##__VA_ARGS__); \
	})
#endif

/*
 * Write an indented warning with EFI prefix
 */
#ifndef EFI_WARN
#define EFI_WARN(format, ...) ({ \
	__EFI_WARN("%sEFI: " format, __efi_nesting(), \
		##__VA_ARGS__); \
	})
#endif

#endif
