#ifndef _ASM_SYSCNT_H_
#define _ASM_SYSCNT_H_

#include <io.h>

#define SYSCNT_CNTCR		0x0000
#define SYSCNT_CNTCR_EN		BIT(0)
#define SYSCNT_CNTCR_HDBG	BIT(1)
#define SYSCNT_CNTCR_FCREQ(n)	BIT(8 + (n))

#define SYSCNT_CNTFID(n)	(0x0020 + 4 * (n))

static inline void syscnt_enable(void __iomem *syscnt)
{
	writel(SYSCNT_CNTCR_EN | SYSCNT_CNTCR_HDBG | SYSCNT_CNTCR_FCREQ(0),
	       syscnt + SYSCNT_CNTCR);
}

static inline u32 syscnt_get_cntfrq(void __iomem *syscnt)
{
	return readl(syscnt + SYSCNT_CNTFID(0));
}

#endif