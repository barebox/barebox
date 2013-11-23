#include <common.h>
#include <io.h>
#include <mach/syslib.h>
#include <mach/omap4-silicon.h>
#include <mach/clocks.h>
#include <mach/omap4-clock.h>

#define LDELAY	12000000

void omap4_configure_mpu_dpll(const struct dpll_param *dpll_param)
{
	/* Unlock the MPU dpll */
	sr32(CM_CLKMODE_DPLL_MPU, 0, 3, PLL_MN_POWER_BYPASS);
	wait_on_value((1 << 0), 0, CM_IDLEST_DPLL_MPU, LDELAY);

	sr32(CM_AUTOIDLE_DPLL_MPU, 0, 3, 0x0); /* Disable DPLL autoidle */

	/* Errata ID: i700, clear CM_CLKSEL_DPLL_MPU[22] : DCC_EN */
	if (omap4_revision() >= OMAP4460_ES1_0)
		sr32(CM_CLKSEL_DPLL_MPU, 0, 22, 0);

	/* Set M,N,M2 values */
	sr32(CM_CLKSEL_DPLL_MPU, 8, 11, dpll_param->m);
	sr32(CM_CLKSEL_DPLL_MPU, 0, 6, dpll_param->n);
	sr32(CM_DIV_M2_DPLL_MPU, 0, 5, dpll_param->m2);
	sr32(CM_DIV_M2_DPLL_MPU, 8, 1, 0x1);

	/* Lock the mpu dpll */
	sr32(CM_CLKMODE_DPLL_MPU, 0, 3, PLL_LOCK | 0x10);
	wait_on_value((1 << 0), 1, CM_IDLEST_DPLL_MPU, LDELAY);
}

void omap4_configure_iva_dpll(const struct dpll_param *dpll_param)
{
	/* Unlock the IVA dpll */
	sr32(CM_CLKMODE_DPLL_IVA, 0, 3, PLL_MN_POWER_BYPASS);
	wait_on_value((1 << 0), 0, CM_IDLEST_DPLL_IVA, LDELAY);

	/* CM_BYPCLK_DPLL_IVA = CORE_X2_CLK/2 */
	sr32(CM_BYPCLK_DPLL_IVA, 0, 2, 0x1);

	sr32(CM_AUTOIDLE_DPLL_IVA, 0, 3, 0x0); /* Disable DPLL autoidle */

	/* Set M,N,M4,M5 */
	sr32(CM_CLKSEL_DPLL_IVA, 8, 11, dpll_param->m);
	sr32(CM_CLKSEL_DPLL_IVA, 0, 7, dpll_param->n);
	sr32(CM_DIV_M4_DPLL_IVA, 0, 5, dpll_param->m4);
	sr32(CM_DIV_M4_DPLL_IVA, 8, 1, 0x1);
	sr32(CM_DIV_M5_DPLL_IVA, 0, 5, dpll_param->m5);
	sr32(CM_DIV_M5_DPLL_IVA, 8, 1, 0x1);

	/* Lock the iva dpll */
	sr32(CM_CLKMODE_DPLL_IVA, 0, 3, PLL_LOCK);
	wait_on_value((1 << 0), 1, CM_IDLEST_DPLL_IVA, LDELAY);
}

void omap4_configure_per_dpll(const struct dpll_param *dpll_param)
{
	/* Unlock the PER dpll */
	sr32(CM_CLKMODE_DPLL_PER, 0, 3, PLL_MN_POWER_BYPASS);
	wait_on_value((1 << 0), 0, CM_IDLEST_DPLL_PER, LDELAY);

	/* Disable autoidle */
	sr32(CM_AUTOIDLE_DPLL_PER, 0, 3, 0x0);

	sr32(CM_CLKSEL_DPLL_PER, 8, 11, dpll_param->m);
	sr32(CM_CLKSEL_DPLL_PER, 0, 6, dpll_param->n);
	sr32(CM_DIV_M2_DPLL_PER, 0, 5, dpll_param->m2);
	sr32(CM_DIV_M3_DPLL_PER, 0, 5, dpll_param->m3);
	sr32(CM_DIV_M4_DPLL_PER, 0, 5, dpll_param->m4);
	sr32(CM_DIV_M5_DPLL_PER, 0, 5, dpll_param->m5);
	sr32(CM_DIV_M6_DPLL_PER, 0, 5, dpll_param->m6);
	sr32(CM_DIV_M7_DPLL_PER, 0, 5, dpll_param->m7);

	sr32(CM_DIV_M2_DPLL_PER, 8, 1, 0x1);
	sr32(CM_DIV_M3_DPLL_PER, 8, 1, 0x1);
	sr32(CM_DIV_M4_DPLL_PER, 8, 1, 0x1);
	sr32(CM_DIV_M5_DPLL_PER, 8, 1, 0x1);
	sr32(CM_DIV_M6_DPLL_PER, 8, 1, 0x1);
	sr32(CM_DIV_M7_DPLL_PER, 8, 1, 0x1);

	/* Lock the per dpll */
	sr32(CM_CLKMODE_DPLL_PER, 0, 3, PLL_LOCK);
	wait_on_value((1 << 0), 1, CM_IDLEST_DPLL_PER, LDELAY);

	return;
}

void omap4_configure_abe_dpll(const struct dpll_param *dpll_param)
{
	/* Select sys_clk as ref clk for ABE dpll */
	writel(0x0, CM_ABE_PLL_REF_CLKSEL);

	/* Unlock the ABE dpll */
	sr32(CM_CLKMODE_DPLL_ABE, 0, 3, PLL_MN_POWER_BYPASS);
	wait_on_value((1 << 0), 0, CM_IDLEST_DPLL_ABE, LDELAY);

	/* Disable autoidle */
	sr32(CM_AUTOIDLE_DPLL_ABE, 0, 3, 0x0);

	sr32(CM_CLKSEL_DPLL_ABE, 8, 11, dpll_param->m);
	sr32(CM_CLKSEL_DPLL_ABE, 0, 6, dpll_param->n);

	/* Force DPLL CLKOUTHIF to stay enabled */
	writel(0x500, CM_DIV_M2_DPLL_ABE);
	sr32(CM_DIV_M2_DPLL_ABE, 0, 5, dpll_param->m2);
	sr32(CM_DIV_M2_DPLL_ABE, 8, 1, 0x1);
	/* Force DPLL CLKOUTHIF to stay enabled */
	writel(0x100, CM_DIV_M3_DPLL_ABE);
	sr32(CM_DIV_M3_DPLL_ABE, 0, 5, dpll_param->m3);
	sr32(CM_DIV_M3_DPLL_ABE, 8, 1, 0x1);

	/* Lock the abe dpll */
	sr32(CM_CLKMODE_DPLL_ABE, 0, 3, PLL_LOCK);
	wait_on_value((1 << 0), 1, CM_IDLEST_DPLL_ABE, LDELAY);

	return;
}

void omap4_configure_usb_dpll(const struct dpll_param *dpll_param)
{
	/* Select the 60Mhz clock 480/8 = 60*/
	writel(0x1, CM_CLKSEL_USB_60MHz);

	/* Unlock the USB dpll */
	sr32(CM_CLKMODE_DPLL_USB, 0, 3, PLL_MN_POWER_BYPASS);
	wait_on_value((1 << 0), 0, CM_IDLEST_DPLL_USB, LDELAY);

	/* Disable autoidle */
	sr32(CM_AUTOIDLE_DPLL_USB, 0, 3, 0x0);

	sr32(CM_CLKSEL_DPLL_USB, 8, 11, dpll_param->m);
	sr32(CM_CLKSEL_DPLL_USB, 0, 6, dpll_param->n);

	/* Force DPLL CLKOUT to stay active */
	writel(0x100, CM_DIV_M2_DPLL_USB);
	sr32(CM_DIV_M2_DPLL_USB, 0, 5, dpll_param->m2);
	sr32(CM_DIV_M2_DPLL_USB, 8, 1, 0x1);
	sr32(CM_CLKDCOLDO_DPLL_USB, 8, 1, 0x1);

	/* Lock the usb dpll */
	sr32(CM_CLKMODE_DPLL_USB, 0, 3, PLL_LOCK);
	wait_on_value((1 << 0), 1, CM_IDLEST_DPLL_USB, LDELAY);

	/* force enable the CLKDCOLDO clock */
	writel(0x100, CM_CLKDCOLDO_DPLL_USB);

	return;
}

void omap4_configure_core_dpll_no_lock(const struct dpll_param *param)
{
	/* CORE_CLK=CORE_X2_CLK/2, L3_CLK=CORE_CLK/2, L4_CLK=L3_CLK/2 */
	writel(0x110, CM_CLKSEL_CORE);

	/* Unlock the CORE dpll */
	sr32(CM_CLKMODE_DPLL_CORE, 0, 3, PLL_MN_POWER_BYPASS);
	wait_on_value((1 << 0), 0, CM_IDLEST_DPLL_CORE, LDELAY);

	/* Disable autoidle */
	sr32(CM_AUTOIDLE_DPLL_CORE, 0, 3, 0x0);

	sr32(CM_CLKSEL_DPLL_CORE, 8, 11, param->m);
	sr32(CM_CLKSEL_DPLL_CORE, 0, 6, param->n);
	sr32(CM_DIV_M2_DPLL_CORE, 0, 5, param->m2);
	sr32(CM_DIV_M3_DPLL_CORE, 0, 5, param->m3);
	sr32(CM_DIV_M4_DPLL_CORE, 0, 5, param->m4);
	sr32(CM_DIV_M5_DPLL_CORE, 0, 5, param->m5);
	sr32(CM_DIV_M6_DPLL_CORE, 0, 5, param->m6);
	sr32(CM_DIV_M7_DPLL_CORE, 0, 5, param->m7);

	sr32(CM_DIV_M2_DPLL_CORE, 8, 1, 0x1);
	sr32(CM_DIV_M3_DPLL_CORE, 8, 1, 0x1);
	sr32(CM_DIV_M4_DPLL_CORE, 8, 1, 0x1);
	sr32(CM_DIV_M5_DPLL_CORE, 8, 1, 0x1);
	sr32(CM_DIV_M6_DPLL_CORE, 8, 1, 0x0);
	sr32(CM_DIV_M7_DPLL_CORE, 8, 1, 0x1);
}

void omap4_lock_core_dpll(void)
{
	/* Lock the core dpll */
	sr32(CM_CLKMODE_DPLL_CORE, 0, 3, PLL_LOCK);
	wait_on_value((1 << 0), 1, CM_IDLEST_DPLL_CORE, LDELAY);

	return;
}

void omap4_lock_core_dpll_shadow(const struct dpll_param *param)
{
	/* Lock the core dpll using freq update method */
	*(volatile int*)0x4A004120 = 10;	//(CM_CLKMODE_DPLL_CORE)

	/* CM_SHADOW_FREQ_CONFIG1: DLL_OVERRIDE = 1(hack), DLL_RESET = 1,
	 * DPLL_CORE_M2_DIV =1, DPLL_CORE_DPLL_EN = 0x7, FREQ_UPDATE = 1
	 */
	*(volatile int*)0x4A004260 = 0x70D | (param->m2 << 11);

	/* Wait for Freq_Update to get cleared: CM_SHADOW_FREQ_CONFIG1 */
	while( ( (*(volatile int*)0x4A004260) & 0x1) == 0x1 );

	/* Wait for DPLL to Lock : CM_IDLEST_DPLL_CORE */
	wait_on_value((1 << 0), 1, CM_IDLEST_DPLL_CORE, LDELAY);
}

void omap4_enable_gpio_clocks(void)
{
	writel(0x1, CM_L4PER_GPIO2_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L4PER_GPIO2_CLKCTRL, LDELAY);
	writel(0x1, CM_L4PER_GPIO3_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L4PER_GPIO3_CLKCTRL, LDELAY);
	writel(0x1, CM_L4PER_GPIO4_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L4PER_GPIO4_CLKCTRL, LDELAY);
	writel(0x1, CM_L4PER_GPIO5_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L4PER_GPIO5_CLKCTRL, LDELAY);
	writel(0x1, CM_L4PER_GPIO6_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L4PER_GPIO6_CLKCTRL, LDELAY);
}

void omap4_enable_gpio1_wup_clocks(void)
{
	/* WKUP clocks */
	writel(0x1, CM_WKUP_GPIO1_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_WKUP_GPIO1_CLKCTRL, LDELAY);
}

void omap4_enable_all_clocks(void)
{
	/* Enable Ducati clocks */
	writel(0x1, CM_DUCATI_DUCATI_CLKCTRL);
	writel(0x2, CM_DUCATI_CLKSTCTRL);

	wait_on_value((1 << 8), (1 << 8), CM_DUCATI_CLKSTCTRL, LDELAY);

	/* Enable ivahd and sl2 clocks */
	writel(0x1, IVAHD_IVAHD_CLKCTRL);
	writel(0x1, IVAHD_SL2_CLKCTRL);
	writel(0x2, IVAHD_CLKSTCTRL);

	wait_on_value((1 << 8), (1 << 8), IVAHD_CLKSTCTRL, LDELAY);

	/* Enable Tesla clocks */
	writel(0x1, DSP_DSP_CLKCTRL);
	writel(0x2, DSP_CLKSTCTRL);

	wait_on_value((1 << 8), (1 << 8), DSP_CLKSTCTRL, LDELAY);

	/* wait for tesla to become accessible */

	/* ABE clocks */
	writel(0x3, CM1_ABE_CLKSTCTRL);
	writel(0x2, CM1_ABE_AESS_CLKCTRL);
	writel(0x2, CM1_ABE_PDM_CLKCTRL);
	writel(0x2, CM1_ABE_DMIC_CLKCTRL);
	writel(0x2, CM1_ABE_MCASP_CLKCTRL);
	writel(0x2, CM1_ABE_MCBSP1_CLKCTRL);
	writel(0x2, CM1_ABE_MCBSP2_CLKCTRL);
	writel(0x2, CM1_ABE_MCBSP3_CLKCTRL);
	writel(0xf02, CM1_ABE_SLIMBUS_CLKCTRL);
	writel(0x2, CM1_ABE_TIMER5_CLKCTRL);
	writel(0x2, CM1_ABE_TIMER6_CLKCTRL);
	writel(0x2, CM1_ABE_TIMER7_CLKCTRL);
	writel(0x2, CM1_ABE_TIMER8_CLKCTRL);
	writel(0x2, CM1_ABE_WDT3_CLKCTRL);
	/* Disable sleep transitions */
	writel(0x0, CM1_ABE_CLKSTCTRL);

	/* L4PER clocks */
	writel(0x2, CM_L4PER_CLKSTCTRL);
	writel(0x2, CM_L4PER_DMTIMER10_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L4PER_DMTIMER10_CLKCTRL, LDELAY);
	writel(0x2, CM_L4PER_DMTIMER11_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L4PER_DMTIMER11_CLKCTRL, LDELAY);
	writel(0x2, CM_L4PER_DMTIMER2_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L4PER_DMTIMER2_CLKCTRL, LDELAY);
	writel(0x2, CM_L4PER_DMTIMER3_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L4PER_DMTIMER3_CLKCTRL, LDELAY);
	writel(0x2, CM_L4PER_DMTIMER4_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L4PER_DMTIMER4_CLKCTRL, LDELAY);
	writel(0x2, CM_L4PER_DMTIMER9_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L4PER_DMTIMER9_CLKCTRL, LDELAY);

	/* GPIO clocks */
	omap4_enable_gpio_clocks();

	writel(0x2, CM_L4PER_HDQ1W_CLKCTRL);

	/* I2C clocks */
	writel(0x2, CM_L4PER_I2C1_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L4PER_I2C1_CLKCTRL, LDELAY);
	writel(0x2, CM_L4PER_I2C2_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L4PER_I2C2_CLKCTRL, LDELAY);
	writel(0x2, CM_L4PER_I2C3_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L4PER_I2C3_CLKCTRL, LDELAY);
	writel(0x2, CM_L4PER_I2C4_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L4PER_I2C4_CLKCTRL, LDELAY);

	writel(0x2, CM_L4PER_MCBSP4_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L4PER_MCBSP4_CLKCTRL, LDELAY);

	/* MCSPI clocks */
	writel(0x2, CM_L4PER_MCSPI1_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L4PER_MCSPI1_CLKCTRL, LDELAY);
	writel(0x2, CM_L4PER_MCSPI2_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L4PER_MCSPI2_CLKCTRL, LDELAY);
	writel(0x2, CM_L4PER_MCSPI3_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L4PER_MCSPI3_CLKCTRL, LDELAY);
	writel(0x2, CM_L4PER_MCSPI4_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L4PER_MCSPI4_CLKCTRL, LDELAY);

	/* MMC clocks */
	sr32(CM_L3INIT_HSMMC1_CLKCTRL, 0, 2, 0x2);
	sr32(CM_L3INIT_HSMMC1_CLKCTRL, 24, 1, 0x1);
	sr32(CM_L3INIT_HSMMC2_CLKCTRL, 0, 2, 0x2);
	sr32(CM_L3INIT_HSMMC2_CLKCTRL, 24, 1, 0x1);
	writel(0x2, CM_L4PER_MMCSD3_CLKCTRL);
	wait_on_value((1 << 18)|(1 << 17)|(1 << 16), 0, CM_L4PER_MMCSD3_CLKCTRL, LDELAY);
	writel(0x2, CM_L4PER_MMCSD4_CLKCTRL);
	wait_on_value((1 << 18)|(1 << 17)|(1 << 16), 0, CM_L4PER_MMCSD4_CLKCTRL, LDELAY);
	writel(0x2, CM_L4PER_MMCSD5_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L4PER_MMCSD5_CLKCTRL, LDELAY);

	/* UART clocks */
	writel(0x2, CM_L4PER_UART1_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L4PER_UART1_CLKCTRL, LDELAY);
	writel(0x2, CM_L4PER_UART2_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L4PER_UART2_CLKCTRL, LDELAY);
	writel(0x2, CM_L4PER_UART3_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L4PER_UART3_CLKCTRL, LDELAY);
	writel(0x2, CM_L4PER_UART4_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L4PER_UART4_CLKCTRL, LDELAY);

	/* WKUP clocks */
	omap4_enable_gpio1_wup_clocks();
	writel(0x01000002, CM_WKUP_TIMER1_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_WKUP_TIMER1_CLKCTRL, LDELAY);

	writel(0x2, CM_WKUP_KEYBOARD_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_WKUP_KEYBOARD_CLKCTRL, LDELAY);

	writel(0x0, CM_SDMA_CLKSTCTRL);
	writel(0x3, CM_MEMIF_CLKSTCTRL);
	writel(0x1, CM_MEMIF_EMIF_1_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_MEMIF_EMIF_1_CLKCTRL, LDELAY);
	writel(0x1, CM_MEMIF_EMIF_2_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_MEMIF_EMIF_2_CLKCTRL, LDELAY);
	writel(0x3, CM_D2D_CLKSTCTRL);
	writel(0x1, CM_L3_2_GPMC_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L3_2_GPMC_CLKCTRL, LDELAY);
	writel(0x1, CM_L3INSTR_L3_3_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L3INSTR_L3_3_CLKCTRL, LDELAY);
	writel(0x1, CM_L3INSTR_L3_INSTR_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L3INSTR_L3_INSTR_CLKCTRL, LDELAY);
	writel(0x1, CM_L3INSTR_OCP_WP1_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_L3INSTR_OCP_WP1_CLKCTRL, LDELAY);

	/* WDT clocks */
	writel(0x2, CM_WKUP_WDT2_CLKCTRL);
	wait_on_value((1 << 17)|(1 << 16), 0, CM_WKUP_WDT2_CLKCTRL, LDELAY);

	/* Enable Camera clocks */
	writel(0x3, CM_CAM_CLKSTCTRL);
	writel(0x102, CM_CAM_ISS_CLKCTRL);
	writel(0x2, CM_CAM_FDIF_CLKCTRL);
	writel(0x0, CM_CAM_CLKSTCTRL);

	/* Enable DSS clocks */
	/* PM_DSS_PWRSTCTRL ON State and LogicState = 1 (Retention) */
	__raw_writel(7, 0x4A307100); /* DSS_PRM */

	writel(0x2, CM_DSS_CLKSTCTRL);
	writel(0xf02, CM_DSS_DSS_CLKCTRL);
	writel(0x2, CM_DSS_DEISS_CLKCTRL);

	/* Check for DSS Clocks */
	while ((__raw_readl(0x4A009100) & 0xF00) != 0xE00)
		;
	/* Set HW_AUTO transition mode */
	writel(0x3, CM_DSS_CLKSTCTRL);

	/* Enable SGX clocks */
	writel(0x2, CM_SGX_CLKSTCTRL);
	writel(0x2, CM_SGX_SGX_CLKCTRL);
	/* Check for SGX FCLK and ICLK */
	while (__raw_readl(0x4A009200) != 0x302)
		;
	/* Enable hsi/unipro/usb clocks */
	writel(0x1, CM_L3INIT_HSI_CLKCTRL);
	writel(0x2, CM_L3INIT_UNIPRO1_CLKCTRL);
	writel(0x2, CM_L3INIT_HSUSBHOST_CLKCTRL);
	writel(0x1, CM_L3INIT_HSUSBOTG_CLKCTRL);
	writel(0x1, CM_L3INIT_HSUSBTLL_CLKCTRL);
	writel(0x2, CM_L3INIT_FSUSB_CLKCTRL);
	/* enable the 32K, 48M optional clocks and enable the module */
	writel(0x301, CM_L3INIT_USBPHY_CLKCTRL);
}

void omap4_do_scale_tps62361(u32 reg, u32 volt_mv)
{
	u32 temp, step;

	step = volt_mv - TPS62361_BASE_VOLT_MV;
	step /= 10;

	temp = TPS62361_I2C_SLAVE_ADDR |
	    (reg << OMAP44XX_PRM_VC_VAL_BYPASS_REGADDR_SHIFT) |
	    (step << OMAP44XX_PRM_VC_VAL_BYPASS_DATA_SHIFT) |
	    OMAP44XX_PRM_VC_VAL_BYPASS_VALID_BIT;
	debug("do_scale_tps62361: volt - %d step - 0x%x\n", volt_mv, step);

	writel(temp, OMAP44XX_PRM_VC_VAL_BYPASS);
	if (!wait_on_value(OMAP44XX_PRM_VC_VAL_BYPASS_VALID_BIT, 0,
				OMAP44XX_PRM_VC_VAL_BYPASS, LDELAY))
		debug("Scaling voltage failed for vdd_mpu from TPS\n");
}
