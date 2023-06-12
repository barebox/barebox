/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __LINUX_PRINTK_H
#define __LINUX_PRINTK_H

#include <linux/list.h>
#include <printk.h>
#include <stdarg.h>

#define MSG_EMERG      0    /* system is unusable */
#define MSG_ALERT      1    /* action must be taken immediately */
#define MSG_CRIT       2    /* critical conditions */
#define MSG_ERR        3    /* error conditions */
#define MSG_WARNING    4    /* warning conditions */
#define MSG_NOTICE     5    /* normal but significant condition */
#define MSG_INFO       6    /* informational */
#define MSG_DEBUG      7    /* debug-level messages */
#define MSG_VDEBUG     8    /* verbose debug messages */

#ifdef VERBOSE_DEBUG
#define LOGLEVEL	MSG_VDEBUG
#elif defined DEBUG
#define LOGLEVEL	MSG_DEBUG
#else
#define LOGLEVEL	CONFIG_COMPILE_LOGLEVEL
#endif

/* debugging and troubleshooting/diagnostic helpers. */
struct device;

#ifndef CONFIG_CONSOLE_NONE
int dev_printf(int level, const struct device *dev, const char *format, ...)
	__attribute__ ((format(__printf__, 3, 4)));
#else
static inline int dev_printf(int level, const struct device *dev,
			     const char *format, ...)
{
	return 0;
}
#endif

#if (!defined(__PBL__) && !defined(CONFIG_CONSOLE_NONE)) || \
	(defined(__PBL__) && defined(CONFIG_PBL_CONSOLE))
int pr_print(int level, const char *format, ...)
	__attribute__ ((format(__printf__, 2, 3)));
#else
static int pr_print(int level, const char *format, ...)
	__attribute__ ((format(__printf__, 2, 3)));
static inline int pr_print(int level, const char *format, ...)
{
	return 0;
}
#endif

#define __dev_printf(level, dev, format, args...) \
	({	\
		(level) <= LOGLEVEL ? dev_printf((level), (dev), (format), ##args) : 0; \
	 })


#define dev_emerg(dev, format, arg...)		\
	__dev_printf(0, (dev) , format , ## arg)
#define dev_alert(dev, format, arg...)		\
	__dev_printf(1, (dev) , format , ## arg)
#define dev_crit(dev, format, arg...)		\
	__dev_printf(2, (dev) , format , ## arg)
#define dev_err(dev, format, arg...)		\
	__dev_printf(3, (dev) , format , ## arg)
#define dev_warn(dev, format, arg...)		\
	__dev_printf(4, (dev) , format , ## arg)
#define dev_notice(dev, format, arg...)		\
	__dev_printf(5, (dev) , format , ## arg)
#define dev_info(dev, format, arg...)		\
	__dev_printf(6, (dev) , format , ## arg)
#define dev_dbg(dev, format, arg...)		\
	__dev_printf(7, (dev) , format , ## arg)
#define dev_vdbg(dev, format, arg...)		\
	__dev_printf(8, (dev) , format , ## arg)

#if LOGLEVEL >= MSG_ERR
int dev_err_probe(struct device *dev, int err, const char *fmt, ...)
	__attribute__ ((format(__printf__, 3, 4)));
#elif !defined(dev_err_probe)
static int dev_err_probe(struct device *dev, int err, const char *fmt, ...)
	__attribute__ ((format(__printf__, 3, 4)));
static inline int dev_err_probe(struct device *dev, int err, const char *fmt,
				...)
{
	return err;
}
#endif

#define __pr_printk(level, format, args...) \
	({	\
		(level) <= LOGLEVEL ? pr_print((level), (format), ##args) : 0; \
	 })

#define __pr_printk_once(level, format, args...) do {	\
	static bool __print_once;			\
							\
	if (!__print_once && (level) <= LOGLEVEL) {	\
		__print_once = true;			\
		pr_print((level), (format), ##args);	\
	}						\
} while (0)

#ifndef pr_fmt
#define pr_fmt(fmt) fmt
#endif

#define pr_emerg(fmt, arg...)	__pr_printk(0, pr_fmt(fmt), ##arg)
#define pr_alert(fmt, arg...)	__pr_printk(1, pr_fmt(fmt), ##arg)
#define pr_crit(fmt, arg...)	__pr_printk(2, pr_fmt(fmt), ##arg)
#define pr_err(fmt, arg...)	__pr_printk(3, pr_fmt(fmt), ##arg)
#define pr_warning(fmt, arg...)	__pr_printk(4, pr_fmt(fmt), ##arg)
#define pr_notice(fmt, arg...)	__pr_printk(5, pr_fmt(fmt), ##arg)
#define pr_info(fmt, arg...)	__pr_printk(6, pr_fmt(fmt), ##arg)
#define pr_debug(fmt, arg...)	__pr_printk(7, pr_fmt(fmt), ##arg)
#define debug(fmt, arg...)	__pr_printk(7, pr_fmt(fmt), ##arg)
#define pr_vdebug(fmt, arg...)	__pr_printk(8, pr_fmt(fmt), ##arg)
#define pr_cont(fmt, arg...)	__pr_printk(-1, fmt, ##arg)

#define pr_emerg_once(fmt, arg...)	__pr_printk_once(0, pr_fmt(fmt), ##arg)
#define pr_alert_once(fmt, arg...)	__pr_printk_once(1, pr_fmt(fmt), ##arg)
#define pr_crit_once(fmt, arg...)	__pr_printk_once(2, pr_fmt(fmt), ##arg)
#define pr_err_once(fmt, arg...)	__pr_printk_once(3, pr_fmt(fmt), ##arg)
#define pr_warning_once(fmt, arg...)	__pr_printk_once(4, pr_fmt(fmt), ##arg)
#define pr_notice_once(fmt, arg...)	__pr_printk_once(5, pr_fmt(fmt), ##arg)
#define pr_info_once(fmt, arg...)	__pr_printk_once(6, pr_fmt(fmt), ##arg)
#define pr_debug_once(fmt, arg...)	__pr_printk_once(7, pr_fmt(fmt), ##arg)
#define pr_vdebug_once(fmt, arg...)	__pr_printk_once(8, pr_fmt(fmt), ##arg)

#define pr_warn			pr_warning
#define pr_warn_once		pr_warning_once

int memory_display(const void *addr, loff_t offs, unsigned nbytes, int size,
		   int swab);
int __pr_memory_display(int level, const void *addr, loff_t offs, unsigned nbytes,
			int size, int swab, const char *format, ...);

#define pr_memory_display(level, addr, offs, nbytes, size, swab) \
	({	\
		(level) <= LOGLEVEL ? __pr_memory_display((level), (addr), \
				(offs), (nbytes), (size), (swab), pr_fmt("")) : 0; \
	 })

struct log_entry {
	struct list_head list;
	char *msg;
	void *dummy;
	uint64_t timestamp;
	int level;
};

extern struct list_head barebox_logbuf;

extern void log_clean(unsigned int limit);

#define BAREBOX_LOG_PRINT_RAW		BIT(2)
#define BAREBOX_LOG_DIFF_TIME		BIT(1)
#define BAREBOX_LOG_PRINT_TIME		BIT(0)

int log_writefile(const char *filepath);
void log_print(unsigned flags, unsigned levels);

struct va_format {
	const char *fmt;
	va_list *va;
};

#if LOGLEVEL >= MSG_DEBUG
#define print_hex_dump_debug(prefix_str, prefix_type, rowsize,		\
			     groupsize, buf, len, ascii)		\
	print_hex_dump(KERN_DEBUG, prefix_str, prefix_type, rowsize,	\
		       groupsize, buf, len, ascii)
#else
static inline void print_hex_dump_debug(const char *prefix_str, int prefix_type,
                                        int rowsize, int groupsize,
                                        const void *buf, size_t len, bool ascii)
{
}
#endif

/**
 * print_hex_dump_bytes - shorthand form of print_hex_dump() with default params
 * @prefix_str: string to prefix each line with;
 *  caller supplies trailing spaces for alignment if desired
 * @prefix_type: controls whether prefix of an offset, address, or none
 *  is printed (%DUMP_PREFIX_OFFSET, %DUMP_PREFIX_ADDRESS, %DUMP_PREFIX_NONE)
 * @buf: data blob to dump
 * @len: number of bytes in the @buf
 *
 * Calls print_hex_dump(), with log level of KERN_DEBUG,
 * rowsize of 16, groupsize of 1, and ASCII output included.
 */
#define print_hex_dump_bytes(prefix_str, prefix_type, buf, len)	\
	print_hex_dump_debug(prefix_str, prefix_type, 16, 1, buf, len, true)

#define dev_WARN_ONCE(dev, condition, format...) ({      \
        static int __warned;			\
        int __ret_warn_once = !!(condition);	\
						\
        if (unlikely(__ret_warn_once)) {	\
                if (!__warned) { 		\
                        __warned = 1;		\
                        dev_warn(dev, format);	\
                }				\
        }					\
        unlikely(__ret_warn_once);		\
})

#endif
