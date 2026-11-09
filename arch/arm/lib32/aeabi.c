// SPDX-License-Identifier: GPL-2.0-only
/*
 * AEABI memory helpers, which rustc emits calls to. The alignment
 * guarantees of the 4/8 variants are ignored.
 */

#include <linux/compiler.h>
#include <linux/types.h>
#include <string.h>

void __aeabi_memcpy(void *dest, const void *src, size_t n);
void __aeabi_memmove(void *dest, const void *src, size_t n);
void __aeabi_memset(void *dest, size_t n, int c);
void __aeabi_memclr(void *dest, size_t n);

void __aeabi_memcpy(void *dest, const void *src, size_t n)
{
	memcpy(dest, src, n);
}
void __aeabi_memcpy4(void *dest, const void *src, size_t n) __alias(__aeabi_memcpy);
void __aeabi_memcpy8(void *dest, const void *src, size_t n) __alias(__aeabi_memcpy);

void __aeabi_memmove(void *dest, const void *src, size_t n)
{
	memmove(dest, src, n);
}
void __aeabi_memmove4(void *dest, const void *src, size_t n) __alias(__aeabi_memmove);
void __aeabi_memmove8(void *dest, const void *src, size_t n) __alias(__aeabi_memmove);

void __aeabi_memset(void *dest, size_t n, int c)
{
	memset(dest, c, n);
}
void __aeabi_memset4(void *dest, size_t n, int c) __alias(__aeabi_memset);
void __aeabi_memset8(void *dest, size_t n, int c) __alias(__aeabi_memset);

void __aeabi_memclr(void *dest, size_t n)
{
	memset(dest, 0, n);
}
void __aeabi_memclr4(void *dest, size_t n) __alias(__aeabi_memclr);
void __aeabi_memclr8(void *dest, size_t n) __alias(__aeabi_memclr);
