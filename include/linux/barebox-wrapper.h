#ifndef __INCLUDE_LINUX_BAREBOX_WRAPPER_H
#define __INCLUDE_LINUX_BAREBOX_WRAPPER_H

#include <malloc.h>
#include <xfuncs.h>
#include <linux/slab.h>

#define vmalloc(len)		malloc(len)
#define __vmalloc(len, mode, pgsz)	malloc(len)
#define vzalloc(len)		kzalloc(len, 0)
static inline void vfree(const void *addr)
{
	free((void *)addr);
}

#define KERN_EMERG      ""   /* system is unusable                   */
#define KERN_ALERT      ""   /* action must be taken immediately     */
#define KERN_CRIT       ""   /* critical conditions                  */
#define KERN_ERR        ""   /* error conditions                     */
#define KERN_WARNING    ""   /* warning conditions                   */
#define KERN_NOTICE     ""   /* normal but significant condition     */
#define KERN_INFO       ""   /* informational                        */
#define KERN_DEBUG      ""   /* debug-level messages                 */
#define KERN_CONT       ""

#define printk			printf

#define pr_warn			pr_warning

#define __init

#define MODULE_AUTHOR(x)
#define MODULE_DESCRIPTION(x)
#define MODULE_LICENSE(x)
#define MODULE_ALIAS(x)

#define __user
#define __init
#define __exit

#define kthread_create(...)	__builtin_return_address(0)
#define kthread_stop(...)	do { } while (0)
#define wake_up_process(...)	do { } while (0)

typedef int irqreturn_t;
#define IRQ_NONE 0
#define IRQ_HANDLED 0

/* To ease clk drivers porting from Linux kernel */
#define __clk_get_name(clk)		(clk->name)
#define __clk_lookup			clk_lookup
#define __clk_get_rate			clk_get_rate
#define __clk_get_parent		clk_get_parent

#endif /* __INCLUDE_LINUX_BAREBOX_WRAPPER_H */
