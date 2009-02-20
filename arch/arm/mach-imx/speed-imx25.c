#include <common.h>
#include <asm/arch/imx-regs.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <init.h>

unsigned long imx_get_mpllclk(void)
{
	ulong mpctl = readl(IMX_CCM_BASE + CCM_MPCTL);
	return imx_decode_pll(mpctl, CONFIG_MX35_HCLK_FREQ);
}

unsigned long imx_get_upllclk(void)
{
	ulong ppctl = readl(IMX_CCM_BASE + CCM_UPCTL);
	return imx_decode_pll(ppctl, CONFIG_MX35_HCLK_FREQ);
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
		fref = imx_get_ipgclk();

	return fref / (val + 1);
}

unsigned long imx_get_perclk_(int per)
{
	ulong ofs = (per & 0x3) * 8;
	ulong reg = per & ~0x3;
	ulong val = (readl(IMX_CCM_BASE + CCM_PCDR0 + reg) >> ofs) & 0x3f;
	ulong fref;

	if (readl(IMX_CCM_BASE + 0x64) & (1 << per))
		fref = imx_get_upllclk();
	else
		fref = imx_get_ipgclk();

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

int imx_dump_clocks(void)
{
	printf("mpll:    %10d Hz\n", imx_get_mpllclk());
	printf("upll:    %10d Hz\n", imx_get_upllclk());
	printf("arm:     %10d Hz\n", imx_get_armclk());
	printf("ahb:     %10d Hz\n", imx_get_ahbclk());
	printf("uart:    %10d Hz\n", imx_get_perclk(0));
	printf("gpt:     %10d Hz\n", imx_get_ipgclk());
	printf("nand:    %10d Hz\n", imx_get_perclk_(8));
	return 0;
}

