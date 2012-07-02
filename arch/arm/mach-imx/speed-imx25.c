#include <common.h>
#include <asm-generic/errno.h>
#include <mach/imx-regs.h>
#include <io.h>
#include <mach/clock.h>
#include <init.h>

unsigned long imx_get_mpllclk(void)
{
	ulong mpctl = readl(IMX_CCM_BASE + CCM_MPCTL);
	return imx_decode_pll(mpctl, CONFIG_MX25_HCLK_FREQ);
}

unsigned long imx_get_upllclk(void)
{
	ulong ppctl = readl(IMX_CCM_BASE + CCM_UPCTL);
	return imx_decode_pll(ppctl, CONFIG_MX25_HCLK_FREQ);
}

unsigned long imx_get_armclk(void)
{
	unsigned long rate, cctl;

	cctl = readl(IMX_CCM_BASE + CCM_CCTL);
	rate = imx_get_mpllclk();

	if (cctl & (1 << 14)) {
		rate *= 3;
		rate >>= 2;
	}

	return rate / ((cctl >> 30) + 1);
}

unsigned long imx_get_ahbclk(void)
{
	ulong cctl = readl(IMX_CCM_BASE + CCM_CCTL);
	return imx_get_armclk() / (((cctl >> 28) & 0x3) + 1);
}

unsigned long imx_get_ipgclk(void)
{
	return imx_get_ahbclk() / 2;
}

unsigned long imx_get_gptclk(void)
{
	return imx_get_ipgclk();
}

unsigned long imx_get_perclk(int per)
{
	ulong ofs = (per & 0x3) * 8;
	ulong reg = per & ~0x3;
	ulong val = (readl(IMX_CCM_BASE + CCM_PCDR0 + reg) >> ofs) & 0x3f;
	ulong fref;

	if (readl(IMX_CCM_BASE + 0x64) & (1 << per))
		fref = imx_get_upllclk();
	else
		fref = imx_get_ahbclk();

	return fref / (val + 1);
}

unsigned long imx_get_uartclk(void)
{
	return imx_get_perclk(15);
}

unsigned long imx_get_fecclk(void)
{
	return imx_get_ipgclk();
}

unsigned long imx_get_lcdclk(void)
{
	return imx_get_perclk(7);
}

unsigned long imx_get_i2cclk(void)
{
	return imx_get_perclk(6);
}

unsigned long imx_get_mmcclk(void)
{
	return imx_get_perclk(3);
}

unsigned long imx_get_cspiclk(void)
{
	return imx_get_ipgclk();
}

void imx_dump_clocks(void)
{
	printf("mpll:    %10ld Hz\n", imx_get_mpllclk());
	printf("upll:    %10ld Hz\n", imx_get_upllclk());
	printf("arm:     %10ld Hz\n", imx_get_armclk());
	printf("ahb:     %10ld Hz\n", imx_get_ahbclk());
	printf("uart:    %10ld Hz\n", imx_get_perclk(15));
	printf("gpt:     %10ld Hz\n", imx_get_ipgclk());
	printf("nand:    %10ld Hz\n", imx_get_perclk(8));
	printf("lcd:     %10ld Hz\n", imx_get_perclk(7));
	printf("i2c:     %10ld Hz\n", imx_get_perclk(6));
	printf("sdhc1:   %10ld Hz\n", imx_get_perclk(3));
}

/*
 * Set the divider of the CLKO pin. Returns
 * the new divider (which may be smaller
 * than the desired one)
 */
int imx_clko_set_div(int num, int div)
{
	unsigned long mcr = readl(IMX_CCM_BASE + 0x64);

	if (num != 1)
		return -ENODEV;

	div -= 1;
	div &= 0x3f;

	mcr &= ~(0x3f << 24);
	mcr |= div << 24;

	writel(mcr, IMX_CCM_BASE + 0x64);

	return div + 1;
}

/*
 * Set the clock source for the CLKO pin
 */
void imx_clko_set_src(int num, int src)
{
	unsigned long mcr = readl(IMX_CCM_BASE + 0x64);

	if (num != 1)
		return;

	if (src < 0) {
		mcr &= ~(1 << 30);
		writel(mcr, IMX_CCM_BASE + 0x64);
		return;
	}

	mcr |= 1 << 30;
	mcr &= ~(0xf << 20);
	mcr |= (src & 0xf) << 20;

	writel(mcr, IMX_CCM_BASE + 0x64);
}

