#ifndef __PRINTK_H
#define __PRINTK_H

#define MSG_EMERG      0    /* system is unusable */
#define MSG_ALERT      1    /* action must be taken immediately */
#define MSG_CRIT       2    /* critical conditions */
#define MSG_ERR        3    /* error conditions */
#define MSG_WARNING    4    /* warning conditions */
#define MSG_NOTICE     5    /* normal but significant condition */
#define MSG_INFO       6    /* informational */
#define MSG_DEBUG      7    /* debug-level messages */

#ifdef DEBUG
#define LOGLEVEL	MSG_DEBUG
#else
#define LOGLEVEL	CONFIG_COMPILE_LOGLEVEL
#endif

/* debugging and troubleshooting/diagnostic helpers. */

int pr_print(int level, const char *format, ...)
	__attribute__ ((format(__printf__, 2, 3)));

int dev_printf(int level, const struct device_d *dev, const char *format, ...)
	__attribute__ ((format(__printf__, 3, 4)));

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

#define __pr_printk(level, format, args...) \
	({	\
		(level) <= LOGLEVEL ? pr_print((level), (format), ##args) : 0; \
	 })

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

#endif
