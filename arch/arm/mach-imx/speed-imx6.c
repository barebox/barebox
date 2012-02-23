#include <common.h>
#include <asm/io.h>
#include <asm-generic/div64.h>
#include <mach/imx-regs.h>
#include <mach/clock-imx6.h>
#include <mach/imx6-anadig.h>

enum pll_clocks {
	CPU_PLL1,	/* System PLL */
	BUS_PLL2,	/* System Bus PLL*/
	USBOTG_PLL3,    /* OTG USB PLL */
	AUD_PLL4,	/* Audio PLL */
	VID_PLL5,	/* Video PLL */
	MLB_PLL6,	/* MLB PLL */
	USBHOST_PLL7,   /* Host USB PLL */
	ENET_PLL8,      /* ENET PLL */
};

#define SZ_DEC_1M		1000000

/* Out-of-reset PFDs and clock source definitions */
#define PLL2_PFD0_FREQ		352000000
#define PLL2_PFD1_FREQ		594000000
#define PLL2_PFD2_FREQ		400000000
#define PLL2_PFD2_DIV_FREQ	200000000
#define PLL3_PFD0_FREQ		720000000
#define PLL3_PFD1_FREQ		540000000
#define PLL3_PFD2_FREQ		508200000
#define PLL3_PFD3_FREQ		454700000
#define PLL3_80M		80000000
#define PLL3_60M		60000000

#define AHB_CLK_ROOT		132000000
#define IPG_CLK_ROOT		66000000
#define ENET_FREQ_0		25000000
#define ENET_FREQ_1		50000000
#define ENET_FREQ_2		100000000
#define ENET_FREQ_3		125000000

#define CONFIG_MX6_HCLK_FREQ      24000000

static u32 __decode_pll(enum pll_clocks pll, u32 infreq)
{
	u32 div;

	switch (pll) {
	case CPU_PLL1:
		div = readl(MX6_ANATOP_BASE_ADDR + HW_ANADIG_PLL_SYS) &
			BM_ANADIG_PLL_SYS_DIV_SELECT;
		return infreq * (div >> 1);
	case BUS_PLL2:
		div = readl(MX6_ANATOP_BASE_ADDR + HW_ANADIG_PLL_528) &
			BM_ANADIG_PLL_528_DIV_SELECT;
		return infreq * (20 + (div << 1));
	case USBOTG_PLL3:
		div = readl(MX6_ANATOP_BASE_ADDR + HW_ANADIG_USB2_PLL_480_CTRL) &
			BM_ANADIG_USB2_PLL_480_CTRL_DIV_SELECT;
		return infreq * (20 + (div << 1));
	case ENET_PLL8:
		div = readl(MX6_ANATOP_BASE_ADDR + HW_ANADIG_PLL_ENET) &
			BM_ANADIG_PLL_ENET_DIV_SELECT;
		switch (div) {
		default:
		case 0:
			return ENET_FREQ_0;
		case 1:
			return ENET_FREQ_1;
		case 2:
			return ENET_FREQ_2;
		case 3:
			return ENET_FREQ_3;
		}
	case AUD_PLL4:
	case VID_PLL5:
	case MLB_PLL6:
	case USBHOST_PLL7:
	default:
		return 0;
	}
}

static u32 __get_mcu_main_clk(void)
{
	u32 reg, freq;
	reg = (__REG(MXC_CCM_CACRR) & MXC_CCM_CACRR_ARM_PODF_MASK) >>
	    MXC_CCM_CACRR_ARM_PODF_OFFSET;
	freq = __decode_pll(CPU_PLL1, CONFIG_MX6_HCLK_FREQ);
	return freq / (reg + 1);
}

static u32 __get_periph_clk(void)
{
	u32 reg;
	reg = __REG(MXC_CCM_CBCDR);
	if (reg & MXC_CCM_CBCDR_PERIPH_CLK_SEL) {
		reg = __REG(MXC_CCM_CBCMR);
		switch ((reg & MXC_CCM_CBCMR_PERIPH_CLK2_SEL_MASK) >>
			MXC_CCM_CBCMR_PERIPH_CLK2_SEL_OFFSET) {
		case 0:
			return __decode_pll(USBOTG_PLL3, CONFIG_MX6_HCLK_FREQ);
		case 1:
		case 2:
			return CONFIG_MX6_HCLK_FREQ;
		default:
			return 0;
		}
	} else {
		reg = __REG(MXC_CCM_CBCMR);
		switch ((reg & MXC_CCM_CBCMR_PRE_PERIPH_CLK_SEL_MASK) >>
			MXC_CCM_CBCMR_PRE_PERIPH_CLK_SEL_OFFSET) {
		default:
		case 0:
			return __decode_pll(BUS_PLL2, CONFIG_MX6_HCLK_FREQ);
		case 1:
			return PLL2_PFD2_FREQ;
		case 2:
			return PLL2_PFD0_FREQ;
		case 3:
			return PLL2_PFD2_DIV_FREQ;
		}
	}
}

static u32 __get_ipg_clk(void)
{
	u32 ahb_podf, ipg_podf;

	ahb_podf = __REG(MXC_CCM_CBCDR);
	ipg_podf = (ahb_podf & MXC_CCM_CBCDR_IPG_PODF_MASK) >>
			MXC_CCM_CBCDR_IPG_PODF_OFFSET;
	ahb_podf = (ahb_podf & MXC_CCM_CBCDR_AHB_PODF_MASK) >>
			MXC_CCM_CBCDR_AHB_PODF_OFFSET;
	return __get_periph_clk() / ((ahb_podf + 1) * (ipg_podf + 1));
}

u32 imx_get_gptclk(void)
{
	return __get_ipg_clk();
}

static u32 __get_ipg_per_clk(void)
{
	u32 podf;
	u32 clk_root = __get_ipg_clk();

	podf = __REG(MXC_CCM_CSCMR1) & MXC_CCM_CSCMR1_PERCLK_PODF_MASK;
	return clk_root / (podf + 1);
}

u32 imx_get_uartclk(void)
{
	u32 freq = PLL3_80M, reg, podf;

	reg = __REG(MXC_CCM_CSCDR1);
	podf = (reg & MXC_CCM_CSCDR1_UART_CLK_PODF_MASK) >>
		MXC_CCM_CSCDR1_UART_CLK_PODF_OFFSET;
	freq /= (podf + 1);

	return freq;
}

static u32 __get_cspi_clk(void)
{
	u32 freq = PLL3_60M, reg, podf;

	reg = __REG(MXC_CCM_CSCDR2);
	podf = (reg & MXC_CCM_CSCDR2_ECSPI_CLK_PODF_MASK) >>
		MXC_CCM_CSCDR2_ECSPI_CLK_PODF_OFFSET;
	freq /= (podf + 1);

	return freq;
}

static u32 __get_axi_clk(void)
{
	u32 clkroot;
	u32 cbcdr =  __REG(MXC_CCM_CBCDR);
	u32 podf = (cbcdr & MXC_CCM_CBCDR_AXI_PODF_MASK) >>
		MXC_CCM_CBCDR_AXI_PODF_OFFSET;

	if (cbcdr & MXC_CCM_CBCDR_AXI_SEL) {
		if (cbcdr & MXC_CCM_CBCDR_AXI_ALT_SEL)
			clkroot = PLL2_PFD2_FREQ;
		else
			clkroot = PLL3_PFD1_FREQ;;
	} else
		clkroot = __get_periph_clk();

	return  clkroot / (podf + 1);
}

static u32 __get_ahb_clk(void)
{
	u32 cbcdr =  __REG(MXC_CCM_CBCDR);
	u32 podf = (cbcdr & MXC_CCM_CBCDR_AHB_PODF_MASK) \
			>> MXC_CCM_CBCDR_AHB_PODF_OFFSET;

	return  __get_periph_clk() / (podf + 1);
}

static u32 __get_emi_slow_clk(void)
{
	u32 cscmr1 =  __REG(MXC_CCM_CSCMR1);
	u32 emi_clk_sel = (cscmr1 & MXC_CCM_CSCMR1_ACLK_EMI_SLOW_MASK) >>
		MXC_CCM_CSCMR1_ACLK_EMI_SLOW_OFFSET;
	u32 podf = (cscmr1 & MXC_CCM_CSCMR1_ACLK_EMI_SLOW_PODF_MASK) >>
			MXC_CCM_CSCMR1_ACLK_EMI_PODF_OFFSET;

	switch (emi_clk_sel) {
	default:
	case 0:
		return __get_axi_clk() / (podf + 1);
	case 1:
		return __decode_pll(USBOTG_PLL3, CONFIG_MX6_HCLK_FREQ) /
			(podf + 1);
	case 2:
		return PLL2_PFD2_FREQ / (podf + 1);
	case 3:
		return PLL2_PFD0_FREQ / (podf + 1);
	}
}

static u32 __get_nfc_clk(void)
{
	u32 clkroot;
	u32 cs2cdr =  __REG(MXC_CCM_CS2CDR);
	u32 podf = (cs2cdr & MXC_CCM_CS2CDR_ENFC_CLK_PODF_MASK) \
			>> MXC_CCM_CS2CDR_ENFC_CLK_PODF_OFFSET;
	u32 pred = (cs2cdr & MXC_CCM_CS2CDR_ENFC_CLK_PRED_MASK) \
			>> MXC_CCM_CS2CDR_ENFC_CLK_PRED_OFFSET;

	switch ((cs2cdr & MXC_CCM_CS2CDR_ENFC_CLK_SEL_MASK) >>
		MXC_CCM_CS2CDR_ENFC_CLK_SEL_OFFSET) {
		default:
		case 0:
			clkroot = PLL2_PFD0_FREQ;
			break;
		case 1:
			clkroot = __decode_pll(BUS_PLL2, CONFIG_MX6_HCLK_FREQ);
			break;
		case 2:
			clkroot = __decode_pll(USBOTG_PLL3, CONFIG_MX6_HCLK_FREQ);
			break;
		case 3:
			clkroot = PLL2_PFD2_FREQ;
			break;
	}

	return  clkroot / (pred+1) / (podf+1);
}

static u32 __get_ddr_clk(void)
{
	u32 cbcdr = __REG(MXC_CCM_CBCDR);
	u32 podf = (cbcdr & MXC_CCM_CBCDR_MMDC_CH0_PODF_MASK) >>
		MXC_CCM_CBCDR_MMDC_CH0_PODF_OFFSET;

	return __get_periph_clk() / (podf + 1);
}

static u32 __get_usdhc1_clk(void)
{
	u32 clkroot;
	u32 cscmr1 = __REG(MXC_CCM_CSCMR1);
	u32 cscdr1 = __REG(MXC_CCM_CSCDR1);
	u32 podf = (cscdr1 & MXC_CCM_CSCDR1_USDHC1_PODF_MASK) >>
		MXC_CCM_CSCDR1_USDHC1_PODF_OFFSET;

	if (cscmr1 & MXC_CCM_CSCMR1_USDHC1_CLK_SEL)
		clkroot = PLL2_PFD0_FREQ;
	else
		clkroot = PLL2_PFD2_FREQ;

	return clkroot / (podf + 1);
}

static u32 __get_usdhc2_clk(void)
{
	u32 clkroot;
	u32 cscmr1 = __REG(MXC_CCM_CSCMR1);
	u32 cscdr1 = __REG(MXC_CCM_CSCDR1);
	u32 podf = (cscdr1 & MXC_CCM_CSCDR1_USDHC2_PODF_MASK) >>
		MXC_CCM_CSCDR1_USDHC2_PODF_OFFSET;

	if (cscmr1 & MXC_CCM_CSCMR1_USDHC2_CLK_SEL)
		clkroot = PLL2_PFD0_FREQ;
	else
		clkroot = PLL2_PFD2_FREQ;

	return clkroot / (podf + 1);
}

static u32 __get_usdhc3_clk(void)
{
	u32 clkroot;
	u32 cscmr1 = __REG(MXC_CCM_CSCMR1);
	u32 cscdr1 = __REG(MXC_CCM_CSCDR1);
	u32 podf = (cscdr1 & MXC_CCM_CSCDR1_USDHC3_PODF_MASK) >>
		MXC_CCM_CSCDR1_USDHC3_PODF_OFFSET;

	if (cscmr1 & MXC_CCM_CSCMR1_USDHC3_CLK_SEL)
		clkroot = PLL2_PFD0_FREQ;
	else
		clkroot = PLL2_PFD2_FREQ;

	return clkroot / (podf + 1);
}

static u32 __get_usdhc4_clk(void)
{
	u32 clkroot;
	u32 cscmr1 = __REG(MXC_CCM_CSCMR1);
	u32 cscdr1 = __REG(MXC_CCM_CSCDR1);
	u32 podf = (cscdr1 & MXC_CCM_CSCDR1_USDHC4_PODF_MASK) >>
		MXC_CCM_CSCDR1_USDHC4_PODF_OFFSET;

	if (cscmr1 & MXC_CCM_CSCMR1_USDHC4_CLK_SEL)
		clkroot = PLL2_PFD0_FREQ;
	else
		clkroot = PLL2_PFD2_FREQ;

	return clkroot / (podf + 1);
}

u32 imx_get_mmcclk(void)
{
	return __get_usdhc3_clk();
}

u32 imx_get_fecclk(void)
{
	return __get_ipg_clk();
}

void imx_dump_clocks(void)
{
	u32 freq;

	freq = __decode_pll(CPU_PLL1, CONFIG_MX6_HCLK_FREQ);
	printf("mx6q pll1: %d\n", freq);
	freq = __decode_pll(BUS_PLL2, CONFIG_MX6_HCLK_FREQ);
	printf("mx6q pll2: %d\n", freq);
	freq = __decode_pll(USBOTG_PLL3, CONFIG_MX6_HCLK_FREQ);
	printf("mx6q pll3: %d\n", freq);
	freq = __decode_pll(ENET_PLL8, CONFIG_MX6_HCLK_FREQ);
	printf("mx6q pll8: %d\n", freq);
	printf("mcu main:  %d\n", __get_mcu_main_clk());
	printf("periph:    %d\n", __get_periph_clk());
	printf("ipg:       %d\n", __get_ipg_clk());
	printf("ipg per:   %d\n", __get_ipg_per_clk());
	printf("cspi:      %d\n", __get_cspi_clk());
	printf("axi:       %d\n", __get_axi_clk());
	printf("ahb:       %d\n", __get_ahb_clk());
	printf("emi slow:  %d\n", __get_emi_slow_clk());
	printf("nfc:       %d\n", __get_nfc_clk());
	printf("ddr:       %d\n", __get_ddr_clk());
	printf("usdhc1:    %d\n", __get_usdhc1_clk());
	printf("usdhc2:    %d\n", __get_usdhc2_clk());
	printf("usdhc3:    %d\n", __get_usdhc3_clk());
	printf("usdhc4:    %d\n", __get_usdhc4_clk());
}

void imx6_ipu_clk_enable(int di)
{
	u32 reg;

	if (di == 1) {
		reg = readl(MXC_CCM_CCGR3);
		reg |= 0xC033;
		writel(reg, MXC_CCM_CCGR3);
	} else {
		reg = readl(MXC_CCM_CCGR3);
		reg |= 0x300F;
		writel(reg, MXC_CCM_CCGR3);
	}

	reg = readl(MX6_ANATOP_BASE_ADDR + 0xF0);
	reg &= ~0x00003F00;
	reg |= 0x00001300;
	writel(reg, MX6_ANATOP_BASE_ADDR + 0xF4);

	reg = readl(MXC_CCM_CS2CDR);
	reg &= ~0x00007E00;
	reg |= 0x00001200;
	writel(reg, MXC_CCM_CS2CDR);

	reg = readl(MXC_CCM_CSCMR2);
	reg |= 0x00000C00;
	writel(reg, MXC_CCM_CSCMR2);

	reg = 0x0002A953;
	writel(reg, MXC_CCM_CHSCDR);
}
