
#ifndef __ASM_ARCH_CLOCK_H
#define __ASM_ARCH_CLOCK_H
unsigned int imx_decode_pll(unsigned int pll, unsigned int f_ref);

ulong imx_get_system_pll(void);
ulong imx_get_fclk(void);
ulong imx_get_hclk(void);
ulong imx_get_BCLK(void);
ulong imx_get_perclk1(void);
ulong imx_get_perclk2(void);
ulong imx_get_perclk3(void);

#endif /* __ASM_ARCH_CLOCK_H */
