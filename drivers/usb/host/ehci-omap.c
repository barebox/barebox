/*
 * Copyright (C) 2010 Michael Grzeschik <mgr@pengutronix.de>
 *
 * This file is released under the GPLv2
 *
 */

/*
 * OMAP USBHOST Register addresses: VIRTUAL ADDRESSES
 */

/*-------------------------------------------------------------------------*/

#include <mfd/twl4030.h>
#include <usb/twl4030.h>
#include <mach/ehci.h>
#include <common.h>
#include <io.h>
#include <clock.h>
#include <gpio.h>
#include <mach/omap3-silicon.h>
#include <mach/omap3-clock.h>
#include <mach/cm-regbits-34xx.h>
#include <mach/sys_info.h>

void omap_usb_utmi_init(struct omap_hcd *omap, u8 tll_channel_mask)
{
	unsigned reg;
	int i;

	/* Program the 3 TLL channels upfront */
	for (i = 0; i < OMAP_TLL_CHANNEL_COUNT; i++) {
		reg = __raw_readl(OMAP3_USBTLL_BASE + OMAP_TLL_CHANNEL_CONF(i));

		/* Disable AutoIdle, BitStuffing and use SDR Mode */
		reg &= ~(OMAP_TLL_CHANNEL_CONF_UTMIAUTOIDLE
				| OMAP_TLL_CHANNEL_CONF_ULPINOBITSTUFF
				| OMAP_TLL_CHANNEL_CONF_ULPIDDRMODE);
		__raw_writel(reg, OMAP3_USBTLL_BASE + OMAP_TLL_CHANNEL_CONF(i));
	}

	/* Program Common TLL register */
	reg = __raw_readl(OMAP3_USBTLL_BASE + OMAP_TLL_SHARED_CONF);
	reg |= (OMAP_TLL_SHARED_CONF_FCLK_IS_ON
			| OMAP_TLL_SHARED_CONF_USB_DIVRATION
			| OMAP_TLL_SHARED_CONF_USB_180D_SDR_EN);
	reg &= ~OMAP_TLL_SHARED_CONF_USB_90D_DDR_EN;

	__raw_writel(reg, OMAP3_USBTLL_BASE + OMAP_TLL_SHARED_CONF);

	/* Enable channels now */
	for (i = 0; i < OMAP_TLL_CHANNEL_COUNT; i++) {
		reg = __raw_readl(OMAP3_USBTLL_BASE + OMAP_TLL_CHANNEL_CONF(i));

		/* Enable only the reg that is needed */
		if (!(tll_channel_mask & 1<<i))
			continue;

		reg |= OMAP_TLL_CHANNEL_CONF_CHANEN;
		__raw_writel(reg, OMAP3_USBTLL_BASE + OMAP_TLL_CHANNEL_CONF(i));

		__raw_writeb(0xbe,
			OMAP3_USBTLL_BASE + OMAP_TLL_ULPI_SCRATCH_REGISTER(i));
	}
}

int ehci_omap_init(struct omap_hcd *omap)
{
	uint64_t start;
	int timeout = 1000;
	u8 tll_ch_mask = 0;
	u32 v = 0;

	if (twl4030_usb_ulpi_init()) {
		printf("ERROR: %s Could not initialize PHY\n",
			__PRETTY_FUNCTION__);
		return -EINVAL;
	}


	v = __raw_readl(OMAP3_CM_REG(CLKSEL4_PLL));
	v |= (12 << OMAP3430ES2_PERIPH2_DPLL_DIV_SHIFT);
	v |= (120 << OMAP3430ES2_PERIPH2_DPLL_MULT_SHIFT);
	__raw_writel(v, OMAP3_CM_REG(CLKSEL4_PLL));

	v = __raw_readl(OMAP3_CM_REG(CLKSEL5_PLL));
	v |= (1 << OMAP3430ES2_DIV_120M_SHIFT);
	__raw_writel(v, OMAP3_CM_REG(CLKSEL5_PLL));

	v = __raw_readl(OMAP3_CM_REG(CLKEN2_PLL));
	v |= (7 << OMAP3430ES2_PERIPH2_DPLL_FREQSEL_SHIFT);
	v |= (7 << OMAP3430ES2_EN_PERIPH2_DPLL_SHIFT);
	__raw_writel(v, OMAP3_CM_REG(CLKEN2_PLL));

	/* PRCM settings for USBHOST:
	* Interface clk un-related to domain transition
	*/

	v = __raw_readl(OMAP3_CM_REG(AIDLE_USBH));
	v |= (0 << OMAP3430ES2_AUTO_USBHOST_SHIFT);
	__raw_writel(v, OMAP3_CM_REG(AIDLE_USBH));

        /* Disable sleep dependency with MPU and IVA */

	v = __raw_readl(OMAP3_CM_REG(SLEEPD_USBH));
	v |= (0 << OMAP3430ES2_EN_MPU_SHIFT);
	v |= (0 << OMAP3430ES2_EN_IVA2_SHIFT);
	__raw_writel(v, OMAP3_CM_REG(SLEEPD_USBH));

	/* Disable Automatic transition of clock */
	v = __raw_readl(OMAP3_CM_REG(CLKSTCTRL_USBH));
	v |= (0 << OMAP3430ES2_CLKTRCTRL_USBHOST_SHIFT);
	__raw_writel(v, OMAP3_CM_REG(CLKSTCTRL_USBH));

	/* Enable Clocks for USBHOST */

	/* enable usbhost_ick */
	v = __raw_readl(OMAP3_CM_REG(ICLKEN_USBH));
	v |= (1 << OMAP3430ES2_EN_USBHOST_SHIFT);
	__raw_writel(v, OMAP3_CM_REG(ICLKEN_USBH));

	/* enable usbhost_120m_fck */
	v = __raw_readl(OMAP3_CM_REG(FCLKEN_USBH));
	v |= (1 << OMAP3430ES2_EN_USBHOST2_SHIFT);
	__raw_writel(v, OMAP3_CM_REG(FCLKEN_USBH));

	/* enable usbhost_48m_fck */
	v = __raw_readl(OMAP3_CM_REG(FCLKEN_USBH));
	v |= (1 << OMAP3430ES2_EN_USBHOST1_SHIFT);
	__raw_writel(v, OMAP3_CM_REG(FCLKEN_USBH));

	if (omap->phy_reset) {
		/* Refer: ISSUE1 */
		if (omap->reset_gpio_port[0] != -EINVAL) {
			gpio_direction_output(omap->reset_gpio_port[0], 0);
		}

		if (omap->reset_gpio_port[1] != -EINVAL) {
			gpio_direction_output(omap->reset_gpio_port[1], 0);
		}

		/* Hold the PHY in RESET for enough time till DIR is high */
		mdelay(10);
	}

	/* enable usbtll_fck  */
	v = __raw_readl(OMAP3_CM_REG(FCLKEN3_CORE));
	v |= (1 << OMAP3430ES2_EN_USBTLL_SHIFT);
	__raw_writel(v, OMAP3_CM_REG(FCLKEN3_CORE));

	/* Configure TLL for 60Mhz clk for ULPI */
	/* enable usbtll_ick */
	v = __raw_readl(OMAP3_CM_REG(ICLKEN3_CORE));
	v |= (1 << OMAP3430ES2_EN_USBTLL_SHIFT);
	__raw_writel(v, OMAP3_CM_REG(ICLKEN3_CORE));

	v = __raw_readl(OMAP3_CM_REG(AIDLE3_CORE));
	v |= (0 << OMAP3430ES2_AUTO_USBTLL_SHIFT);
	__raw_writel(v, OMAP3_CM_REG(AIDLE3_CORE));

	/* perform TLL soft reset, and wait until reset is complete */
	__raw_writel(OMAP_USBTLL_SYSCONFIG_SOFTRESET,
				OMAP3_USBTLL_BASE + OMAP_USBTLL_SYSCONFIG);

	/* Wait for TLL reset to complete */
	start = get_time_ns();

	while (!(__raw_readl(OMAP3_USBTLL_BASE + OMAP_USBTLL_SYSSTATUS)
			& OMAP_USBTLL_SYSSTATUS_RESETDONE)) {
		if (is_timeout(start, timeout * USECOND)) {
			return -ETIMEDOUT;
		}
	}

	/* (1<<3) = no idle mode only for initial debugging */
	__raw_writel(OMAP_USBTLL_SYSCONFIG_ENAWAKEUP |
			OMAP_USBTLL_SYSCONFIG_SIDLEMODE |
			OMAP_USBTLL_SYSCONFIG_CACTIVITY, OMAP3_USBTLL_BASE + OMAP_USBTLL_SYSCONFIG);

	/* Put UHH in NoIdle/NoStandby mode */
	v = __raw_readl(OMAP3_UHH_CONFIG_BASE + OMAP_UHH_SYSCONFIG);
	v |= (OMAP_UHH_SYSCONFIG_ENAWAKEUP
			| OMAP_UHH_SYSCONFIG_SIDLEMODE
			| OMAP_UHH_SYSCONFIG_CACTIVITY
			| OMAP_UHH_SYSCONFIG_MIDLEMODE);
	v &= ~OMAP_UHH_SYSCONFIG_AUTOIDLE;
	__raw_writel(v, OMAP3_UHH_CONFIG_BASE + OMAP_UHH_SYSCONFIG);

	v = __raw_readl(OMAP3_UHH_CONFIG_BASE + OMAP_UHH_HOSTCONFIG);
	/* setup ULPI bypass and burst configurations */
	v |= (OMAP_UHH_HOSTCONFIG_INCR4_BURST_EN
			| OMAP_UHH_HOSTCONFIG_INCR8_BURST_EN
			| OMAP_UHH_HOSTCONFIG_INCR16_BURST_EN);
	v &= ~OMAP_UHH_HOSTCONFIG_INCRX_ALIGN_EN;

	if (omap->port_mode[0] == EHCI_HCD_OMAP_MODE_UNKNOWN)
		v &= ~OMAP_UHH_HOSTCONFIG_P1_CONNECT_STATUS;
	if (omap->port_mode[1] == EHCI_HCD_OMAP_MODE_UNKNOWN)
		v &= ~OMAP_UHH_HOSTCONFIG_P2_CONNECT_STATUS;
	if (omap->port_mode[2] == EHCI_HCD_OMAP_MODE_UNKNOWN)
		v &= ~OMAP_UHH_HOSTCONFIG_P3_CONNECT_STATUS;

	/* Bypass the TLL module for PHY mode operation */
	 if (get_cpu_rev() <= OMAP34XX_ES2_1) {
		if ((omap->port_mode[0] == EHCI_HCD_OMAP_MODE_PHY) ||
			(omap->port_mode[1] == EHCI_HCD_OMAP_MODE_PHY) ||
				(omap->port_mode[2] == EHCI_HCD_OMAP_MODE_PHY))
			v &= ~OMAP_UHH_HOSTCONFIG_ULPI_BYPASS;
		else
			v |= OMAP_UHH_HOSTCONFIG_ULPI_BYPASS;
	} else {
		if (omap->port_mode[0] == EHCI_HCD_OMAP_MODE_PHY)
			v &= ~OMAP_UHH_HOSTCONFIG_ULPI_P1_BYPASS;
		else if (omap->port_mode[0] == EHCI_HCD_OMAP_MODE_TLL)
			v |= OMAP_UHH_HOSTCONFIG_ULPI_P1_BYPASS;

		if (omap->port_mode[1] == EHCI_HCD_OMAP_MODE_PHY)
			v &= ~OMAP_UHH_HOSTCONFIG_ULPI_P2_BYPASS;
		else if (omap->port_mode[1] == EHCI_HCD_OMAP_MODE_TLL)
			v |= OMAP_UHH_HOSTCONFIG_ULPI_P2_BYPASS;

		if (omap->port_mode[2] == EHCI_HCD_OMAP_MODE_PHY)
			v &= ~OMAP_UHH_HOSTCONFIG_ULPI_P3_BYPASS;
		else if (omap->port_mode[2] == EHCI_HCD_OMAP_MODE_TLL)
			v |= OMAP_UHH_HOSTCONFIG_ULPI_P3_BYPASS;

	}
	__raw_writel(v, OMAP3_UHH_CONFIG_BASE + OMAP_UHH_HOSTCONFIG);

	if ((omap->port_mode[0] == EHCI_HCD_OMAP_MODE_TLL) ||
		(omap->port_mode[1] == EHCI_HCD_OMAP_MODE_TLL) ||
			(omap->port_mode[2] == EHCI_HCD_OMAP_MODE_TLL)) {

		if (omap->port_mode[0] == EHCI_HCD_OMAP_MODE_TLL)
			tll_ch_mask |= OMAP_TLL_CHANNEL_1_EN_MASK;
		if (omap->port_mode[1] == EHCI_HCD_OMAP_MODE_TLL)
			tll_ch_mask |= OMAP_TLL_CHANNEL_2_EN_MASK;
		if (omap->port_mode[2] == EHCI_HCD_OMAP_MODE_TLL)
			tll_ch_mask |= OMAP_TLL_CHANNEL_3_EN_MASK;

		/* Enable UTMI mode for required TLL channels */
		omap_usb_utmi_init(omap, tll_ch_mask);
	}

	if (omap->phy_reset) {
		/* Refer ISSUE1:
		 * Hold the PHY in RESET for enough time till
		 * PHY is settled and ready
		 */
		udelay(10);

		if (omap->reset_gpio_port[0] != -EINVAL)
			gpio_direction_output(omap->reset_gpio_port[0], 1);

		if (omap->reset_gpio_port[1] != -EINVAL)
			gpio_direction_output(omap->reset_gpio_port[1], 1);
	}

	return 0;
}
