#ifndef __INCLUDE_ASM_ARCH_IMX_PLL_H
#define __INCLUDE_ASM_ARCH_IMX_PLL_H

/*
 * This can be used for various PLLs found on
 * i.MX SoCs.
 *
 *                   mfi + mfn / (mfd + 1)
 * fpll = 2 * fref * ---------------------
 *                          pd + 1
 */
#define IMX_PLL_PD(x)		(((x) & 0xf) << 26)
#define IMX_PLL_MFD(x)		(((x) & 0x3ff) << 16)
#define IMX_PLL_MFI(x)		(((x) & 0xf) << 10)
#define IMX_PLL_MFN(x)		(((x) & 0x3ff) << 0)
#define IMX_PLL_BRMO		(1 << 31)

#endif /* __INCLUDE_ASM_ARCH_IMX_PLL_H*/
