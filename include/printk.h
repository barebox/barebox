/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __PRINTK_H
#define __PRINTK_H

#define KERN_EMERG      ""   /* system is unusable                   */
#define KERN_ALERT      ""   /* action must be taken immediately     */
#define KERN_CRIT       ""   /* critical conditions                  */
#define KERN_ERR        ""   /* error conditions                     */
#define KERN_WARNING    ""   /* warning conditions                   */
#define KERN_NOTICE     ""   /* normal but significant condition     */
#define KERN_INFO       ""   /* informational                        */
#define KERN_DEBUG      ""   /* debug-level messages                 */
#define KERN_CONT       ""

#if (!defined(__PBL__) && !defined(CONFIG_CONSOLE_NONE)) || \
	(defined(__PBL__) && defined(CONFIG_PBL_CONSOLE))
int printf(const char *fmt, ...) __attribute__ ((format(__printf__, 1, 2)));
#else
static int printf(const char *fmt, ...) __attribute__ ((format(__printf__, 1, 2)));
static inline int printf(const char *fmt, ...)
{
	return 0;
}
#endif

#define printk			printf

#define printk_once(fmt, ...)					\
({								\
	static bool __print_once	;			\
								\
	if (!__print_once) {					\
		__print_once = true;				\
		printk(fmt, ##__VA_ARGS__);			\
	}							\
})

enum {
	DUMP_PREFIX_NONE,
	DUMP_PREFIX_ADDRESS,
	DUMP_PREFIX_OFFSET
};
extern int hex_dump_to_buffer(const void *buf, size_t len, int rowsize,
			      int groupsize, char *linebuf, size_t linebuflen,
			      bool ascii);
extern void print_hex_dump(const char *level, const char *prefix_str,
			   int prefix_type, int rowsize, int groupsize,
			   const void *buf, size_t len, bool ascii);

#ifdef CONFIG_ARCH_HAS_STACK_DUMP
void dump_stack(void);
#else
static inline void dump_stack(void)
{
	printf("no stack data available\n");
}
#endif


#endif
