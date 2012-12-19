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

/* debugging and troubleshooting/diagnostic helpers. */

int dev_printf(const struct device_d *dev, const char *format, ...)
	__attribute__ ((format(__printf__, 2, 3)));


#define dev_emerg(dev, format, arg...)		\
	dev_printf((dev) , format , ## arg)
#define dev_alert(dev, format, arg...)		\
	dev_printf((dev) , format , ## arg)
#define dev_crit(dev, format, arg...)		\
	dev_printf((dev) , format , ## arg)
#define dev_err(dev, format, arg...)		\
	dev_printf(dev , format , ## arg)
#define dev_warn(dev, format, arg...)		\
	dev_printf((dev) , format , ## arg)
#define dev_notice(dev, format, arg...)		\
	dev_printf((dev) , format , ## arg)
#define dev_info(dev, format, arg...)		\
	dev_printf((dev) , format , ## arg)

#if defined(DEBUG)
#define dev_dbg(dev, format, arg...)		\
	dev_printf((dev) , format , ## arg)
#else
#define dev_dbg(dev, format, arg...)		\
	({ if (0) dev_printf((dev), format, ##arg); 0; })
#endif

#define pr_info(fmt, arg...)	printk(fmt, ##arg)
#define pr_notice(fmt, arg...)	printk(fmt, ##arg)
#define pr_err(fmt, arg...)	printk(fmt, ##arg)
#define pr_warning(fmt, arg...)	printk(fmt, ##arg)
#define pr_crit(fmt, arg...)	printk(fmt, ##arg)
#define pr_alert(fmt, arg...)	printk(fmt, ##arg)
#define pr_emerg(fmt, arg...)	printk(fmt, ##arg)

#ifdef DEBUG
#define pr_debug(fmt, arg...)	printk(fmt, ##arg)
#define debug(fmt, arg...)	printf(fmt, ##arg)
#else
#define pr_debug(fmt, arg...)	do {} while(0)
#define debug(fmt, arg...)	do {} while(0)
#endif

#endif
