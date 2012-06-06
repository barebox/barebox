#ifndef __IO_H
#define __IO_H

#include <asm/io.h>

/* cpu_read/cpu_write: cpu native io accessors */
#define cpu_readb(a)		__raw_readb(a)
#define cpu_readw(a)		__raw_readw(a)
#define cpu_readl(a)		__raw_readl(a)
#define cpu_writeb(v, a)	__raw_writeb((v), (a))
#define cpu_writew(v, a)	__raw_writew((v), (a))
#define cpu_writel(v, a)	__raw_writel((v), (a))

#endif /* __IO_H */
