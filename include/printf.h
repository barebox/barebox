/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __PRINTF_H
#define __PRINTF_H

#include <linux/types.h>
#include <linux/compiler.h>

struct device;

#define KERN_EMERG      ""   /* system is unusable                   */
#define KERN_ALERT      ""   /* action must be taken immediately     */
#define KERN_CRIT       ""   /* critical conditions                  */
#define KERN_ERR        ""   /* error conditions                     */
#define KERN_WARNING    ""   /* warning conditions                   */
#define KERN_NOTICE     ""   /* normal but significant condition     */
#define KERN_INFO       ""   /* informational                        */
#define KERN_DEBUG      ""   /* debug-level messages                 */
#define KERN_CONT       ""

#if (IN_PROPER && !defined(CONFIG_CONSOLE_NONE)) || \
	(IN_PBL && defined(CONFIG_PBL_CONSOLE))
int printf(const char *fmt, ...) __printf(1, 2);
#else
static inline __printf(1, 2) int printf(const char *fmt, ...)
{
	return 0;
}
#endif

void __noreturn panic(const char *fmt, ...) __printf(1, 2);
void __noreturn panic_no_stacktrace(const char *fmt, ...) __printf(1, 2);

#define printk			printf

enum {
	DUMP_PREFIX_NONE,
	DUMP_PREFIX_ADDRESS,
	DUMP_PREFIX_OFFSET
};
extern int hex_dump_to_buffer(const void *buf, size_t len, int rowsize,
			      int groupsize, char *linebuf, size_t linebuflen,
			      bool ascii);
extern void dev_print_hex_dump(struct device *dev, const char *level,
			       const char *prefix_str, int prefix_type,
			       int rowsize, int groupsize, const void *buf,
			       size_t len, bool ascii);

#define print_hex_dump(level, prefix_str, prefix_type, rowsize,		       \
			     groupsize, buf, len, ascii)		       \
	dev_print_hex_dump(NULL, level, prefix_str, prefix_type, rowsize,      \
		       groupsize, buf, len, ascii)

#ifdef CONFIG_ARCH_HAS_STACK_DUMP
void dump_stack(void);
#else
static inline void dump_stack(void)
{
	printf("no stack data available\n");
}
#endif


#endif
