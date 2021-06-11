/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __IO_H
#define __IO_H

#include <asm/io.h>

#define IOMEM_ERR_PTR(err) (__force void __iomem *)ERR_PTR(err)

#ifndef readq_relaxed
#define readq_relaxed(addr) readq(addr)
#endif

#ifndef readl_relaxed
#define readl_relaxed(addr) readl(addr)
#endif

#ifndef readw_relaxed
#define readw_relaxed(addr) readw(addr)
#endif

#ifndef readb_relaxed
#define readb_relaxed(addr) readb(addr)
#endif

#ifndef writeq_relaxed
#define writeq_relaxed(val, addr) writeq((val), (addr))
#endif

#ifndef writel_relaxed
#define writel_relaxed(val, addr) writel((val), (addr))
#endif

#ifndef writew_relaxed
#define writew_relaxed(val, addr) writew((val), (addr))
#endif

#ifndef writeb_relaxed
#define writeb_relaxed(val, addr) writeb((val), (addr))
#endif

#endif /* __IO_H */
