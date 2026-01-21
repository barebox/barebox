/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_KERNEL_SPRINTF_H_
#define _LINUX_KERNEL_SPRINTF_H_

#include <linux/types.h>
#include <stdarg.h>

int sprintf(char *buf, const char *fmt, ...) __printf(2, 3);
int snprintf(char *buf, size_t size, const char *fmt, ...) __printf(3, 4);
int scnprintf(char *buf, size_t size, const char *fmt, ...) __printf(3, 4);
int vsprintf(char *buf, const char *fmt, va_list args);
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
int vscnprintf(char *buf, size_t size, const char *fmt, va_list args);

#if IN_PROPER || defined(CONFIG_PBL_CONSOLE)
int asprintf(char **strp, const char *fmt, ...) __printf(2, 3);
char *bvasprintf(const char *fmt, va_list ap);
int vasprintf(char **strp, const char *fmt, va_list ap);
int vrasprintf(char **strp, const char *fmt, va_list ap);
int rasprintf(char **strp, const char *fmt, ...) __printf(2, 3);
#else
static inline __printf(2, 3) int asprintf(char **strp, const char *fmt, ...)
{
	return -1;
}
static inline char *bvasprintf(const char *fmt, va_list ap)
{
	return NULL;
}
static inline int vasprintf(char **strp, const char *fmt, va_list ap)
{
	return -1;
}
static inline int vrasprintf(char **strp, const char *fmt, va_list ap)
{
	return -1;
}
static inline __printf(2, 3) int rasprintf(char **strp, const char *fmt, ...)
{
	return -1;
}
#endif

#define basprintf xasprintf

char *uuid_string(char *buf, const char *end, const u8 *addr, int field_width,
		  int precision, int flags, const char *fmt);
const char *size_human_readable(unsigned long long size);


#endif	/* _LINUX_KERNEL_SPRINTF_H */
