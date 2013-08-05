#ifndef __INCLUDE_LINUX_BAREBOX_WRAPPER_H
#define __INCLUDE_LINUX_BAREBOX_WRAPPER_H

#include <xfuncs.h>

#define kmalloc(len, mode)	malloc(len)
#define kzalloc(len, mode)	xzalloc(len)
#define vmalloc(len)		malloc(len)
#define kfree(ptr)		free(ptr)
#define vzalloc(len)		kzalloc(len, 0)
#define vfree(ptr)		free(ptr)

#define KERN_EMERG      ""   /* system is unusable                   */
#define KERN_ALERT      ""   /* action must be taken immediately     */
#define KERN_CRIT       ""   /* critical conditions                  */
#define KERN_ERR        ""   /* error conditions                     */
#define KERN_WARNING    ""   /* warning conditions                   */
#define KERN_NOTICE     ""   /* normal but significant condition     */
#define KERN_INFO       ""   /* informational                        */
#define KERN_DEBUG      ""   /* debug-level messages                 */
#define KERN_CONT       ""

#define GFP_KERNEL	0

typedef int     gfp_t;

#define printk			printf

#define pr_warn			pr_warning

#define __init

#define MODULE_AUTHOR(x)
#define MODULE_DESCRIPTION(x)
#define MODULE_LICENSE(x)

#endif /* __INCLUDE_LINUX_BAREBOX_WRAPPER_H */
