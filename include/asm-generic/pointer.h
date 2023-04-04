/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ASM_GENERIC_PTR_H___
#define __ASM_GENERIC_PTR_H___

#if __SIZEOF_POINTER__ == 8
#ifdef __ASSEMBLY__
#define ASM_PTR		.quad
#define ASM_SZPTR	8
#define ASM_LGPTR	3
#else
#define ASM_PTR		".quad"
#define ASM_SZPTR	"8"
#define ASM_LGPTR	"3"
#endif
#elif __SIZEOF_POINTER__ == 4
#ifdef __ASSEMBLY__
#define ASM_PTR		.word
#define ASM_SZPTR	4
#define ASM_LGPTR	2
#else
#define ASM_PTR		".word"
#define ASM_SZPTR	"4"
#define ASM_LGPTR	"2"
#endif
#else
#error "Unexpected __SIZEOF_POINTER__"
#endif

#endif /* __ASM_GENERIC_PTR_H___ */
