
#ifndef __ASM_ARCH_CLOCK_H
#define __ASM_ARCH_CLOCK_H
unsigned int imx_decode_pll(unsigned int pll, unsigned int f_ref);

ulong imx_get_mpllclk(void);

#ifdef CONFIG_ARCH_IMX27
ulong imx_get_armclk(void);
#endif
#ifdef CONFIG_ARCH_IMX1
static inline ulong imx_get_armclk(void)
{
	return imx_get_mpllclk();
}
#endif

ulong imx_get_spllclk(void);
ulong imx_get_fclk(void);
ulong imx_get_hclk(void);
ulong imx_get_bclk(void);
ulong imx_get_perclk1(void);
ulong imx_get_perclk2(void);
ulong imx_get_perclk3(void);
ulong imx_get_ahbclk(void);
ulong imx_get_fecclk(void);
ulong imx_get_gptclk(void);
ulong imx_get_uartclk(void);
ulong imx_get_lcdclk(void);
ulong imx_get_i2cclk(void);
ulong imx_get_mmcclk(void);
ulong imx_get_cspiclk(void);
ulong imx_get_ipgclk(void);
ulong imx_get_usbclk(void);

int imx_clko_set_div(int num, int div);
void imx_clko_set_src(int num, int src);

void imx_dump_clocks(void);

#endif /* __ASM_ARCH_CLOCK_H */
