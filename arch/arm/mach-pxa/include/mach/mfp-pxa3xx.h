#ifndef __ASM_ARCH_MFP_PXA3XX_H
#define __ASM_ARCH_MFP_PXA3XX_H

#include <plat/mfp.h>

#define MFPR_BASE	(0x40e10000)

/* NOTE: usage of these two functions is not recommended,
 * use pxa3xx_mfp_config() instead.
 */
static inline unsigned long pxa3xx_mfp_read(int mfp)
{
	return mfp_read(mfp);
}

static inline void pxa3xx_mfp_write(int mfp, unsigned long val)
{
	mfp_write(mfp, val);
}

static inline void pxa3xx_mfp_config(unsigned long *mfp_cfg, int num)
{
	mfp_config(mfp_cfg, num);
}
#endif /* __ASM_ARCH_MFP_PXA3XX_H */
