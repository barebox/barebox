#ifndef __IO_H
#define __IO_H

#include <asm/io.h>

/* cpu_read/cpu_write: cpu native io accessors */
#if __BYTE_ORDER == __BIG_ENDIAN
#define cpu_readb(a)		readb(a)
#define cpu_readw(a)		in_be16(a)
#define cpu_readl(a)		in_be32(a)
#define cpu_writeb(v, a)	writeb((v), (a))
#define cpu_writew(v, a)	out_be16((a), (v))
#define cpu_writel(v, a)	out_be32((a), (v))
#else
#define cpu_readb(a)		readb(a)
#define cpu_readw(a)		readw(a)
#define cpu_readl(a)		readl(a)
#define cpu_writeb(v, a)	writeb((v), (a))
#define cpu_writew(v, a)	writew((v), (a))
#define cpu_writel(v, a)	writel((v), (a))
#endif

#endif /* __IO_H */
