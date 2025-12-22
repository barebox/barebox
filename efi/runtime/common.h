/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __EFI_RUNTME_H_
#define __EFI_RUNTME_H_

/* For potential use in headers */
#define __EFI_RUNTIME__

/* We use objcopy(1) to prefix, so avoid double prefixing here */
#define __efi_runtime
#define __efi_runtime_data
#define __efi_runtime_rodata
#define EFI_RUNTIME_SECTION(sect) sect

#ifdef CONFIG_DEBUG_EFI_RUNTIME_ENTRY
#include <debug_ll.h>

#define EFI_ENTRY(wstr, ...)	do {	\
	puts_ll(__func__);		\
	puts_ll("(");			\
	putws_ll(wstr ?: L"<NULL>");	\
	puts_ll(")");			\
} while (0)
#define EFI_EXIT(ret)		({	\
	puts_ll("\t= ");		\
	puthex_ll((ulong)ret);		\
	puts_ll("\n");			\
	(ret);				\
})
#else
#define EFI_ENTRY(...)		(void)0
#define EFI_EXIT(ret)		({ ret; })
#endif /* CONFIG_DEBUG_EFI_RUNTIME_ENTRY */

#endif
