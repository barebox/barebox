// SPDX-License-Identifier: GPL-2.0-or-later
#include "dwc2.h"

/* Returns the controller's GHWCFG2.OTG_MODE. */
static unsigned int dwc2_op_mode(struct dwc2 *dwc2)
{
	u32 ghwcfg2 = dwc2_readl(dwc2, GHWCFG2);

	return (ghwcfg2 & GHWCFG2_OP_MODE_MASK) >>
		GHWCFG2_OP_MODE_SHIFT;
}

/* Returns true if the controller is host-only. */
static bool dwc2_hw_is_host(struct dwc2 *dwc2)
{
	unsigned int op_mode = dwc2_op_mode(dwc2);

	return (op_mode == GHWCFG2_OP_MODE_SRP_CAPABLE_HOST) ||
		(op_mode == GHWCFG2_OP_MODE_NO_SRP_CAPABLE_HOST);
}

/* Returns true if the controller is device-only. */
static bool dwc2_hw_is_device(struct dwc2 *dwc2)
{
	unsigned int op_mode = dwc2_op_mode(dwc2);

	return (op_mode == GHWCFG2_OP_MODE_SRP_CAPABLE_DEVICE) ||
		(op_mode == GHWCFG2_OP_MODE_NO_SRP_CAPABLE_DEVICE);
}

static void dwc2_set_param_otg_cap(struct dwc2 *dwc2)
{
	u8 val;

	switch (dwc2->hw_params.op_mode) {
	case GHWCFG2_OP_MODE_HNP_SRP_CAPABLE:
		val = DWC2_CAP_PARAM_HNP_SRP_CAPABLE;
		break;
	case GHWCFG2_OP_MODE_SRP_ONLY_CAPABLE:
	case GHWCFG2_OP_MODE_SRP_CAPABLE_DEVICE:
	case GHWCFG2_OP_MODE_SRP_CAPABLE_HOST:
		val = DWC2_CAP_PARAM_SRP_ONLY_CAPABLE;
		break;
	default:
		val = DWC2_CAP_PARAM_NO_HNP_SRP_CAPABLE;
		break;
	}

	dwc2->params.otg_cap = val;
}

static void dwc2_set_param_phy_type(struct dwc2 *dwc2)
{
	u8 val;

	switch (dwc2->hw_params.hs_phy_type) {
	case GHWCFG2_HS_PHY_TYPE_UTMI:
	case GHWCFG2_HS_PHY_TYPE_UTMI_ULPI:
		val = DWC2_PHY_TYPE_PARAM_UTMI;
		break;
	case GHWCFG2_HS_PHY_TYPE_ULPI:
		val = DWC2_PHY_TYPE_PARAM_ULPI;
		break;
	default:
		dwc2_warn(dwc2, "Unhandled HS PHY type\n");
		fallthrough;
	case GHWCFG2_HS_PHY_TYPE_NOT_SUPPORTED:
		val = DWC2_PHY_TYPE_PARAM_FS;
		break;
	}

	dwc2->params.phy_type = val;
}

static void dwc2_set_param_speed(struct dwc2 *dwc2)
{
	if (dwc2->params.phy_type == DWC2_PHY_TYPE_PARAM_FS)
		dwc2->params.speed = DWC2_SPEED_PARAM_FULL;
	else
		dwc2->params.speed = DWC2_SPEED_PARAM_HIGH;
}

static void dwc2_set_param_phy_utmi_width(struct dwc2 *dwc2)
{
	int val;

	val = (dwc2->hw_params.utmi_phy_data_width ==
	       GHWCFG4_UTMI_PHY_DATA_WIDTH_8) ? 8 : 16;

	dwc2->params.phy_utmi_width = val;
}

/**
 * dwc2_hsotg_tx_fifo_count - return count of TX FIFOs in device mode
 *
 * @hsotg: Programming view of the DWC_otg controller
 */
int dwc2_tx_fifo_count(struct dwc2 *dwc2)
{
	if (dwc2->hw_params.en_multiple_tx_fifo)
		/* In dedicated FIFO mode we need count of IN EPs */
		return dwc2->hw_params.num_dev_in_eps;
	else
		/* In shared FIFO mode we need count of Periodic IN EPs */
		return dwc2->hw_params.num_dev_perio_in_ep;
}

static void dwc2_set_param_fifo_sizes(struct dwc2 *dwc2)
{
	struct dwc2_hw_params *hw = &dwc2->hw_params;
	struct dwc2_core_params *p = &dwc2->params;
	u32 total_fifo_size = dwc2->hw_params.total_fifo_size;
	u32 max_np_tx_fifo_size = hw->dev_nperio_tx_fifo_size;
	u32 max_rx_fifo_size = hw->rx_fifo_size;
	u32 depth;
	int count, i;

	count = dwc2_tx_fifo_count(dwc2);

	depth = total_fifo_size / 4;
	p->g_np_tx_fifo_size = min(max_np_tx_fifo_size, depth);
	total_fifo_size -= p->g_np_tx_fifo_size;

	depth = 8 * count + 256;
	depth = max(total_fifo_size / count, depth);
	p->g_rx_fifo_size = min(max_rx_fifo_size, depth);
	total_fifo_size -= p->g_rx_fifo_size;

	for (i = 1; i <= count; i++)
		p->g_tx_fifo_size[i] = total_fifo_size / count;
}

/**
 * dwc2_set_default_params() - Set all core parameters to their
 * auto-detected default values.
 *
 * @dwc2: Programming view of the DWC2 controller
 *
 */
void dwc2_set_default_params(struct dwc2 *dwc2)
{
	struct dwc2_hw_params *hw = &dwc2->hw_params;
	struct dwc2_core_params *p = &dwc2->params;
	bool dma_capable = !(hw->arch == GHWCFG2_SLAVE_ONLY_ARCH);

	dwc2_set_param_otg_cap(dwc2);
	dwc2_set_param_phy_type(dwc2);
	dwc2_set_param_speed(dwc2);
	dwc2_set_param_phy_utmi_width(dwc2);
	p->phy_ulpi_ddr = false;
	p->phy_ulpi_ext_vbus = false;

	p->enable_dynamic_fifo = hw->enable_dynamic_fifo;
	p->en_multiple_tx_fifo = hw->en_multiple_tx_fifo;
	p->i2c_enable = hw->i2c_enable;
	p->acg_enable = hw->acg_enable;
	p->ulpi_fs_ls = false;
	p->ts_dline = false;
	p->reload_ctl = (hw->snpsid >= DWC2_CORE_REV_2_92a);
	p->uframe_sched = true;
	p->external_id_pin_ctl = false;
	p->lpm = true;
	p->lpm_clock_gating = true;
	p->besl = true;
	p->hird_threshold_en = true;
	p->hird_threshold = 4;
	p->ipg_isoc_en = false;
	p->max_packet_count = hw->max_packet_count;
	p->max_transfer_size = hw->max_transfer_size;
	p->ahbcfg = GAHBCFG_HBSTLEN_INCR << GAHBCFG_HBSTLEN_SHIFT;

	p->dma = dma_capable;
	p->dma_desc = false;

	if (dwc2->dr_mode == USB_DR_MODE_HOST ||
	    dwc2->dr_mode == USB_DR_MODE_OTG) {
		p->host_support_fs_ls_low_power = false;
		p->host_ls_low_power_phy_clk = false;
		p->host_channels = hw->host_channels;
		p->host_rx_fifo_size = hw->rx_fifo_size;
		p->host_nperio_tx_fifo_size = hw->host_nperio_tx_fifo_size;
		p->host_perio_tx_fifo_size = hw->host_perio_tx_fifo_size;
	}

	if ((dwc2->dr_mode == USB_DR_MODE_PERIPHERAL) ||
	    (dwc2->dr_mode == USB_DR_MODE_OTG)) {
		dwc2_set_param_fifo_sizes(dwc2);
	}
}

void dwc2_get_device_properties(struct dwc2 *dwc2)
{
	struct dwc2_core_params *p = &dwc2->params;
	struct device_node *np = dwc2->dev->of_node;
	int num;

	if ((dwc2->dr_mode == USB_DR_MODE_PERIPHERAL) ||
	    (dwc2->dr_mode == USB_DR_MODE_OTG)) {
		of_property_read_u32(np, "g-rx-fifo-size",
					 &p->g_rx_fifo_size);

		of_property_read_u32(np, "g-np-tx-fifo-size",
					 &p->g_np_tx_fifo_size);

		num = of_property_count_elems_of_size(np, "g-tx-fifo-size", sizeof(u32));
		if (num > 0) {
			num = min(num, 15);
			memset(p->g_tx_fifo_size, 0,
			       sizeof(p->g_tx_fifo_size));
			of_property_read_u32_array(np,
						       "g-tx-fifo-size",
						       &p->g_tx_fifo_size[1],
						       num);
		}
	}
}

int dwc2_check_core_version(struct dwc2 *dwc2)
{
	struct dwc2_hw_params *hw = &dwc2->hw_params;

	/*
	 * Attempt to ensure this device is really a DWC2 Controller.
	 * Read and verify the GSNPSID register contents. The value should be
	 * 0x4f54xxxx, 0x5531xxxx or 0x5532xxxx
	 */
	hw->snpsid = dwc2_readl(dwc2, GSNPSID);
	if ((hw->snpsid & GSNPSID_ID_MASK) != DWC2_OTG_ID &&
	    (hw->snpsid & GSNPSID_ID_MASK) != DWC2_FS_IOT_ID &&
	    (hw->snpsid & GSNPSID_ID_MASK) != DWC2_HS_IOT_ID) {
		dwc2_err(dwc2, "Bad value for GSNPSID: 0x%08x\n",
			hw->snpsid);
		return -ENODEV;
	}

	dwc2_dbg(dwc2, "Core Release: %1x.%1x%1x%1x (snpsid=%x)\n",
		hw->snpsid >> 12 & 0xf, hw->snpsid >> 8 & 0xf,
		hw->snpsid >> 4 & 0xf, hw->snpsid & 0xf, hw->snpsid);

	return 0;
}

static void dwc2_get_dev_hwparams(struct dwc2 *dwc2)
{
	struct dwc2_hw_params *hw = &dwc2->hw_params;
	u32 size;
	int count, i;

	size = FIFOSIZE_DEPTH_GET(dwc2_readl(dwc2, GNPTXFSIZ));
	hw->dev_nperio_tx_fifo_size = size;

	count = dwc2_tx_fifo_count(dwc2);

	for (i = 1; i <= count; i++) {
		size = FIFOSIZE_DEPTH_GET(dwc2_readl(dwc2, DPTXFSIZN(i)));
		hw->g_tx_fifo_size[i] = size;
	}
}

/**
 * During device initialization, read various hardware configuration
 * registers and interpret the contents.
 *
 * @dwc2: Programming view of the DWC2 controller
 *
 */
void dwc2_get_hwparams(struct dwc2 *dwc2)
{
	struct dwc2_hw_params *hw = &dwc2->hw_params;
	unsigned int width;
	u32 hwcfg1, hwcfg2, hwcfg3, hwcfg4;
	u32 grxfsiz;

	hwcfg1 = dwc2_readl(dwc2, GHWCFG1);
	hwcfg2 = dwc2_readl(dwc2, GHWCFG2);
	hwcfg3 = dwc2_readl(dwc2, GHWCFG3);
	hwcfg4 = dwc2_readl(dwc2, GHWCFG4);
	grxfsiz = dwc2_readl(dwc2, GRXFSIZ);

	/* hwcfg1 */
	hw->dev_ep_dirs = hwcfg1;

	/* hwcfg2 */
	hw->op_mode = (hwcfg2 & GHWCFG2_OP_MODE_MASK) >>
		      GHWCFG2_OP_MODE_SHIFT;
	hw->arch = (hwcfg2 & GHWCFG2_ARCHITECTURE_MASK) >>
		   GHWCFG2_ARCHITECTURE_SHIFT;
	hw->enable_dynamic_fifo = !!(hwcfg2 & GHWCFG2_DYNAMIC_FIFO);
	hw->host_channels = 1 + ((hwcfg2 & GHWCFG2_NUM_HOST_CHAN_MASK) >>
				GHWCFG2_NUM_HOST_CHAN_SHIFT);
	hw->hs_phy_type = (hwcfg2 & GHWCFG2_HS_PHY_TYPE_MASK) >>
			  GHWCFG2_HS_PHY_TYPE_SHIFT;
	hw->fs_phy_type = (hwcfg2 & GHWCFG2_FS_PHY_TYPE_MASK) >>
			  GHWCFG2_FS_PHY_TYPE_SHIFT;
	hw->num_dev_ep = (hwcfg2 & GHWCFG2_NUM_DEV_EP_MASK) >>
			 GHWCFG2_NUM_DEV_EP_SHIFT;
	hw->nperio_tx_q_depth =
		(hwcfg2 & GHWCFG2_NONPERIO_TX_Q_DEPTH_MASK) >>
		GHWCFG2_NONPERIO_TX_Q_DEPTH_SHIFT << 1;
	hw->host_perio_tx_q_depth =
		(hwcfg2 & GHWCFG2_HOST_PERIO_TX_Q_DEPTH_MASK) >>
		GHWCFG2_HOST_PERIO_TX_Q_DEPTH_SHIFT << 1;
	hw->dev_token_q_depth =
		(hwcfg2 & GHWCFG2_DEV_TOKEN_Q_DEPTH_MASK) >>
		GHWCFG2_DEV_TOKEN_Q_DEPTH_SHIFT;

	/* hwcfg3 */
	width = (hwcfg3 & GHWCFG3_XFER_SIZE_CNTR_WIDTH_MASK) >>
		GHWCFG3_XFER_SIZE_CNTR_WIDTH_SHIFT;
	hw->max_transfer_size = (1 << (width + 11)) - 1;
	width = (hwcfg3 & GHWCFG3_PACKET_SIZE_CNTR_WIDTH_MASK) >>
		GHWCFG3_PACKET_SIZE_CNTR_WIDTH_SHIFT;
	hw->max_packet_count = (1 << (width + 4)) - 1;
	hw->i2c_enable = !!(hwcfg3 & GHWCFG3_I2C);
	hw->total_fifo_size = (hwcfg3 & GHWCFG3_DFIFO_DEPTH_MASK) >>
			      GHWCFG3_DFIFO_DEPTH_SHIFT;
	hw->lpm_mode = !!(hwcfg3 & GHWCFG3_OTG_LPM_EN);

	/* hwcfg4 */
	hw->en_multiple_tx_fifo = !!(hwcfg4 & GHWCFG4_DED_FIFO_EN);
	hw->num_dev_perio_in_ep = (hwcfg4 & GHWCFG4_NUM_DEV_PERIO_IN_EP_MASK) >>
				  GHWCFG4_NUM_DEV_PERIO_IN_EP_SHIFT;
	hw->num_dev_in_eps = (hwcfg4 & GHWCFG4_NUM_IN_EPS_MASK) >>
			     GHWCFG4_NUM_IN_EPS_SHIFT;
	hw->dma_desc_enable = !!(hwcfg4 & GHWCFG4_DESC_DMA);
	hw->power_optimized = !!(hwcfg4 & GHWCFG4_POWER_OPTIMIZ);
	hw->hibernation = !!(hwcfg4 & GHWCFG4_HIBER);
	hw->utmi_phy_data_width = (hwcfg4 & GHWCFG4_UTMI_PHY_DATA_WIDTH_MASK) >>
				  GHWCFG4_UTMI_PHY_DATA_WIDTH_SHIFT;
	hw->acg_enable = !!(hwcfg4 & GHWCFG4_ACG_SUPPORTED);
	hw->ipg_isoc_en = !!(hwcfg4 & GHWCFG4_IPG_ISOC_SUPPORTED);

	/* fifo sizes */
	hw->rx_fifo_size = (grxfsiz & GRXFSIZ_DEPTH_MASK) >>
				GRXFSIZ_DEPTH_SHIFT;

	dwc2_get_dev_hwparams(dwc2);
}

/*
 * Initializes the FSLSPClkSel field of the HCFG register depending on the
 * PHY type
 */
void dwc2_init_fs_ls_pclk_sel(struct dwc2 *dwc2)
{
	u32 hcfg, val;

	if ((dwc2->hw_params.hs_phy_type == GHWCFG2_HS_PHY_TYPE_ULPI &&
	     dwc2->hw_params.fs_phy_type == GHWCFG2_FS_PHY_TYPE_DEDICATED &&
	     dwc2->params.ulpi_fs_ls) ||
	    dwc2->params.phy_type == DWC2_PHY_TYPE_PARAM_FS) {
		/* Full speed PHY */
		val = HCFG_FSLSPCLKSEL_48_MHZ;
	} else {
		/* High speed PHY running at full speed or high speed */
		val = HCFG_FSLSPCLKSEL_30_60_MHZ;
	}

	dwc2_dbg(dwc2, "Initializing HCFG.FSLSPClkSel to %08x\n", val);
	hcfg = dwc2_readl(dwc2, HCFG);
	hcfg &= ~HCFG_FSLSPCLKSEL_MASK;
	hcfg |= val << HCFG_FSLSPCLKSEL_SHIFT;
	dwc2_writel(dwc2, hcfg, HCFG);
}

void dwc2_flush_all_fifo(struct dwc2 *dwc2)
{
	uint32_t greset;

	/* Wait for AHB master IDLE state */
	if (dwc2_wait_bit_set(dwc2, GRSTCTL, GRSTCTL_AHBIDLE, 10000)) {
		dwc2_warn(dwc2, "%s: Timeout waiting for AHB Idle\n", __func__);
		return;
	}

	greset = GRSTCTL_TXFFLSH | GRSTCTL_RXFFLSH;
	/* TXFNUM of 0x10 is to flush all TX FIFO */
	dwc2_writel(dwc2, greset | GRSTCTL_TXFNUM(0x10), GRSTCTL);

	/* Wait for TxFIFO and RxFIFO flush done */
	if (dwc2_wait_bit_clear(dwc2, GRSTCTL, greset, 10000))
		dwc2_warn(dwc2, "Timeout flushing fifos (GRSTCTL=%08x)\n",
			 dwc2_readl(dwc2, GRSTCTL));

	/* Wait for 3 PHY Clocks */
	udelay(1);
}

/**
 * dwc2_flush_tx_fifo() - Flushes a Tx FIFO
 *
 * @hsotg: Programming view of DWC_otg controller
 * @idx: The fifo index (0..15)
 */
void dwc2_flush_tx_fifo(struct dwc2 *dwc2, const int idx)
{
	u32 greset;

	if (idx > 15)
		return;

	dwc2_dbg(dwc2, "Flush Tx FIFO %d\n", idx);

	/* Wait for AHB master IDLE state */
	if (dwc2_wait_bit_set(dwc2, GRSTCTL, GRSTCTL_AHBIDLE, 10000)) {
		dwc2_warn(dwc2, "%s: Timeout waiting for AHB Idle\n", __func__);
		return;
	}

	greset = GRSTCTL_TXFFLSH;
	greset |= GRSTCTL_TXFNUM(idx) & GRSTCTL_TXFNUM_MASK;
	dwc2_writel(dwc2, greset, GRSTCTL);

	if (dwc2_wait_bit_clear(dwc2, GRSTCTL, GRSTCTL_TXFFLSH, 10000))
		dwc2_warn(dwc2, "%s: Timeout flushing tx fifo (GRSTCTL=%08x)\n",
			 __func__, dwc2_readl(dwc2, GRSTCTL));

	/* Wait for at least 3 PHY Clocks */
	udelay(1);
}

static int dwc2_fs_phy_init(struct dwc2 *dwc2, bool select_phy)
{
	u32 usbcfg, ggpio, i2cctl;
	int retval = 0;

	/*
	 * core_init() is now called on every switch so only call the
	 * following for the first time through
	 */
	if (select_phy) {
		dwc2_dbg(dwc2, "FS PHY selected\n");

		usbcfg = dwc2_readl(dwc2, GUSBCFG);
		if (!(usbcfg & GUSBCFG_PHYSEL)) {
			usbcfg |= GUSBCFG_PHYSEL;
			dwc2_writel(dwc2, usbcfg, GUSBCFG);

			/* Reset after a PHY select */
			retval = dwc2_core_reset(dwc2);

			if (retval) {
				dwc2_err(dwc2,
					"%s: Reset failed, aborting", __func__);
				return retval;
			}
		}

		if (dwc2->params.activate_stm_fs_transceiver) {
			ggpio = dwc2_readl(dwc2, GGPIO);
			if (!(ggpio & GGPIO_STM32_OTG_GCCFG_PWRDWN)) {
				dwc2_dbg(dwc2, "Activating transceiver\n");
				/*
				 * STM32F4x9 uses the GGPIO register as general
				 * core configuration register.
				 */
				ggpio |= GGPIO_STM32_OTG_GCCFG_PWRDWN;
				dwc2_writel(dwc2, ggpio, GGPIO);
			}
		}
	}

	if (dwc2->params.i2c_enable) {
		dwc2_dbg(dwc2, "FS PHY enabling I2C\n");

		/* Program GUSBCFG.OtgUtmiFsSel to I2C */
		usbcfg = dwc2_readl(dwc2, GUSBCFG);
		usbcfg |= GUSBCFG_OTG_UTMI_FS_SEL;
		dwc2_writel(dwc2, usbcfg, GUSBCFG);

		/* Program GI2CCTL.I2CEn */
		i2cctl = dwc2_readl(dwc2, GI2CCTL);
		i2cctl &= ~GI2CCTL_I2CDEVADDR_MASK;
		i2cctl |= 1 << GI2CCTL_I2CDEVADDR_SHIFT;
		i2cctl &= ~GI2CCTL_I2CEN;
		dwc2_writel(dwc2, i2cctl, GI2CCTL);
		i2cctl |= GI2CCTL_I2CEN;
		dwc2_writel(dwc2, i2cctl, GI2CCTL);
	}

	return retval;
}

static int dwc2_hs_phy_init(struct dwc2 *dwc2, bool select_phy)
{
	u32 usbcfg, usbcfg_old;
	int retval = 0;

	if (!select_phy)
		return 0;

	usbcfg = dwc2_readl(dwc2, GUSBCFG);
	usbcfg_old = usbcfg;

	/*
	 * HS PHY parameters. These parameters are preserved during soft reset
	 * so only program the first time. Do a soft reset immediately after
	 * setting phyif.
	 */
	switch (dwc2->params.phy_type) {
	case DWC2_PHY_TYPE_PARAM_ULPI:
		/* ULPI interface */
		dwc2_dbg(dwc2, "HS ULPI PHY selected\n");
		usbcfg |= GUSBCFG_ULPI_UTMI_SEL;
		usbcfg &= ~(GUSBCFG_PHYIF16 | GUSBCFG_DDRSEL | GUSBCFG_PHYSEL);
		if (dwc2->params.phy_ulpi_ddr)
			usbcfg |= GUSBCFG_DDRSEL;

		/* Set external VBUS indicator as needed. */
		if (dwc2->params.phy_ulpi_ext_vbus_ind) {
			dwc2_dbg(dwc2, "Use external VBUS indicator\n");
			usbcfg |= GUSBCFG_ULPI_EXT_VBUS_IND;
			usbcfg &= ~GUSBCFG_INDICATORCOMPLEMENT;
			usbcfg &= ~GUSBCFG_INDICATORPASSTHROUGH;

			if (dwc2->params.phy_ulpi_ext_vbus_ind_complement)
				usbcfg |= GUSBCFG_INDICATORCOMPLEMENT;
			if (dwc2->params.phy_ulpi_ext_vbus_ind_passthrough)
				usbcfg |= GUSBCFG_INDICATORPASSTHROUGH;
		}
		break;
	case DWC2_PHY_TYPE_PARAM_UTMI:
		/* UTMI+ interface */
		dwc2_dbg(dwc2, "HS UTMI+ PHY selected\n");
		usbcfg &= ~(GUSBCFG_ULPI_UTMI_SEL | GUSBCFG_PHYIF16);
		if (dwc2->params.phy_utmi_width == 16)
			usbcfg |= GUSBCFG_PHYIF16;
		break;
	default:
		dwc2_err(dwc2, "FS PHY selected at HS!\n");
		break;
	}

	if (usbcfg != usbcfg_old) {
		dwc2_writel(dwc2, usbcfg, GUSBCFG);

		/* Reset after setting the PHY parameters */
		retval = dwc2_core_reset(dwc2);
		if (retval) {
			dwc2_err(dwc2,
				"%s: Reset failed, aborting", __func__);
			return retval;
		}
	}

	return retval;
}

int dwc2_phy_init(struct dwc2 *dwc2, bool select_phy)
{
	u32 usbcfg;
	int retval = 0;

	if ((dwc2->params.speed == DWC2_SPEED_PARAM_FULL ||
	     dwc2->params.speed == DWC2_SPEED_PARAM_LOW) &&
	    dwc2->params.phy_type == DWC2_PHY_TYPE_PARAM_FS) {
		/* If FS/LS mode with FS/LS PHY */
		retval = dwc2_fs_phy_init(dwc2, select_phy);
		if (retval)
			return retval;
	} else {
		/* High speed PHY */
		retval = dwc2_hs_phy_init(dwc2, select_phy);
		if (retval)
			return retval;
	}

	usbcfg = dwc2_readl(dwc2, GUSBCFG);
	usbcfg &= ~GUSBCFG_ULPI_FS_LS;
	usbcfg &= ~GUSBCFG_ULPI_CLK_SUSP_M;
	if (dwc2->hw_params.hs_phy_type == GHWCFG2_HS_PHY_TYPE_ULPI &&
	    dwc2->hw_params.fs_phy_type == GHWCFG2_FS_PHY_TYPE_DEDICATED &&
	    dwc2->params.ulpi_fs_ls) {
		dwc2_dbg(dwc2, "Setting ULPI FSLS\n");
		usbcfg |= GUSBCFG_ULPI_FS_LS;
		usbcfg |= GUSBCFG_ULPI_CLK_SUSP_M;
	}
	dwc2_writel(dwc2, usbcfg, GUSBCFG);

	return retval;
}

int dwc2_gahbcfg_init(struct dwc2 *dwc2)
{
	u32 ahbcfg = dwc2_readl(dwc2, GAHBCFG);

	switch (dwc2->hw_params.arch) {
	case GHWCFG2_EXT_DMA_ARCH:
		dwc2_err(dwc2, "External DMA Mode not supported\n");
		return -EINVAL;

	case GHWCFG2_INT_DMA_ARCH:
		dwc2_dbg(dwc2, "Internal DMA Mode\n");
		if (dwc2->params.ahbcfg != -1) {
			ahbcfg &= GAHBCFG_CTRL_MASK;
			ahbcfg |= dwc2->params.ahbcfg &
				  ~GAHBCFG_CTRL_MASK;
		}
		break;

	case GHWCFG2_SLAVE_ONLY_ARCH:
	default:
		dwc2_dbg(dwc2, "Slave Only Mode\n");
		break;
	}

	if (dwc2->params.dma)
		ahbcfg |= GAHBCFG_DMA_EN;
	else
		dwc2->params.dma_desc = false;

	dwc2_writel(dwc2, ahbcfg, GAHBCFG);

	return 0;
}

void dwc2_gusbcfg_init(struct dwc2 *dwc2)
{
	u32 usbcfg;

	usbcfg = dwc2_readl(dwc2, GUSBCFG);
	usbcfg &= ~(GUSBCFG_HNPCAP | GUSBCFG_SRPCAP);

	switch (dwc2->hw_params.op_mode) {
	case GHWCFG2_OP_MODE_HNP_SRP_CAPABLE:
		if (dwc2->params.otg_cap ==
				DWC2_CAP_PARAM_HNP_SRP_CAPABLE)
			usbcfg |= GUSBCFG_HNPCAP;

	case GHWCFG2_OP_MODE_SRP_ONLY_CAPABLE:
	case GHWCFG2_OP_MODE_SRP_CAPABLE_DEVICE:
	case GHWCFG2_OP_MODE_SRP_CAPABLE_HOST:
		if (dwc2->params.otg_cap !=
				DWC2_CAP_PARAM_NO_HNP_SRP_CAPABLE)
			usbcfg |= GUSBCFG_SRPCAP;
		break;

	case GHWCFG2_OP_MODE_NO_HNP_SRP_CAPABLE:
	case GHWCFG2_OP_MODE_NO_SRP_CAPABLE_DEVICE:
	case GHWCFG2_OP_MODE_NO_SRP_CAPABLE_HOST:
	default:
		break;
	}

	dwc2_writel(dwc2, usbcfg, GUSBCFG);
}

/*
 * Check the dr_mode against the module configuration and hardware
 * capabilities.
 *
 * The hardware, module, and dr_mode, can each be set to host, device,
 * or otg. Check that all these values are compatible and adjust the
 * value of dr_mode if possible.
 *
 *                      actual
 *    HW  MOD dr_mode   dr_mode
 *  ------------------------------
 *   HST  HST  any    :  HST
 *   HST  DEV  any    :  ---
 *   HST  OTG  any    :  HST
 *
 *   DEV  HST  any    :  ---
 *   DEV  DEV  any    :  DEV
 *   DEV  OTG  any    :  DEV
 *
 *   OTG  HST  any    :  HST
 *   OTG  DEV  any    :  DEV
 *   OTG  OTG  any    :  dr_mode
 */
int dwc2_get_dr_mode(struct dwc2 *dwc2)
{
	enum usb_dr_mode mode;

	mode = of_usb_get_dr_mode(dwc2->dev->of_node, NULL);
	dwc2->dr_mode = mode;

	if (dwc2_hw_is_device(dwc2)) {
		dwc2_dbg(dwc2, "Controller is device only\n");
		if (!IS_ENABLED(CONFIG_USB_DWC2_GADGET)) {
			dwc2_err(dwc2, "gadget mode support not compiled in!\n");
			return -ENOTSUPP;
		}
		mode = USB_DR_MODE_PERIPHERAL;
	} else if (dwc2_hw_is_host(dwc2)) {
		dwc2_dbg(dwc2, "Controller is host only\n");
		if (!IS_ENABLED(CONFIG_USB_DWC2_HOST)) {
			dwc2_err(dwc2, "host mode support not compiled in!\n");
			return -ENOTSUPP;
		}
		mode = USB_DR_MODE_HOST;
	} else {
		dwc2_dbg(dwc2, "Controller is otg\n");
		if (IS_ENABLED(CONFIG_USB_DWC2_HOST) &&
		    IS_ENABLED(CONFIG_USB_DWC2_GADGET))
			mode = dwc2->dr_mode;
		else if (IS_ENABLED(CONFIG_USB_DWC2_HOST))
			mode = USB_DR_MODE_HOST;
		else if (IS_ENABLED(CONFIG_USB_DWC2_GADGET))
			mode = USB_DR_MODE_PERIPHERAL;
	}

	if (mode != dwc2->dr_mode) {
		dwc2_warn(dwc2,
			 "Selected dr_mode not supported by controller/driver. Enforcing '%s' mode.\n",
			mode == USB_DR_MODE_HOST ? "host" : "peripheral");

		dwc2->dr_mode = mode;
	}

	return 0;
}

/**
 * dwc2_wait_for_mode() - Waits for the controller mode.
 * @dwc2:	Programming view of the DWC_otg controller.
 * @host_mode:	If true, waits for host mode, otherwise device mode.
 */
void dwc2_wait_for_mode(struct dwc2 *dwc2, bool host_mode)
{
	unsigned int timeout = 110 * MSECOND;
	int ret;

	dev_vdbg(dwc2->dev, "Waiting for %s mode\n",
		 host_mode ? "host" : "device");

	ret = wait_on_timeout(timeout, dwc2_is_host_mode(dwc2) == host_mode);
	if (ret)
		dev_err(dwc2->dev, "%s: Couldn't set %s mode\n",
				 __func__, host_mode ? "host" : "device");

	dev_vdbg(dwc2->dev, "%s mode set\n",
		 host_mode ? "Host" : "Device");
}

/**
 * dwc2_iddig_filter_enabled() - Returns true if the IDDIG debounce
 * filter is enabled.
 *
 * @hsotg: Programming view of DWC_otg controller
 */
bool dwc2_iddig_filter_enabled(struct dwc2 *dwc2)
{
	u32 gsnpsid;
	u32 ghwcfg4;

	/* Check if core configuration includes the IDDIG filter. */
	ghwcfg4 = dwc2_readl(dwc2, GHWCFG4);
	if (!(ghwcfg4 & GHWCFG4_IDDIG_FILT_EN))
		return false;

	/*
	 * Check if the IDDIG debounce filter is bypassed. Available
	 * in core version >= 3.10a.
	 */
	gsnpsid = dwc2_readl(dwc2, GSNPSID);
	if (gsnpsid >= DWC2_CORE_REV_3_10a) {
		u32 gotgctl = dwc2_readl(dwc2, GOTGCTL);

		if (gotgctl & GOTGCTL_DBNCE_FLTR_BYPASS)
			return false;
	}

	return true;
}

/*
 * Do core a soft reset of the core.  Be careful with this because it
 * resets all the internal state machines of the core.
 */
int dwc2_core_reset(struct dwc2 *dwc2)
{
	bool wait_for_host_mode = false;
	uint32_t greset;
	int ret;

	dwc2_dbg(dwc2, "%s(%p)\n", __func__, dwc2);

	/* Wait for AHB master IDLE state. */
	ret = dwc2_wait_bit_set(dwc2, GRSTCTL, GRSTCTL_AHBIDLE, 10000);
	if (ret) {
		dwc2_warn(dwc2, "%s: Timeout! Waiting for AHB master IDLE state\n",
				__func__);
		return ret;
	}

	/*
	 * If the current mode is host, either due to the force mode
	 * bit being set (which persists after core reset) or the
	 * connector id pin, a core soft reset will temporarily reset
	 * the mode to device. A delay from the IDDIG debounce filter
	 * will occur before going back to host mode.
	 *
	 * Determine whether we will go back into host mode after a
	 * reset and account for this delay after the reset.
	 */
	if (dwc2_iddig_filter_enabled(dwc2)) {
		u32 gotgctl = dwc2_readl(dwc2, GOTGCTL);
		u32 gusbcfg = dwc2_readl(dwc2, GUSBCFG);

		if (!(gotgctl & GOTGCTL_CONID_B) ||
		    (gusbcfg & GUSBCFG_FORCEHOSTMODE)) {
			wait_for_host_mode = true;
		}
	}

	/* Core Soft Reset */
	greset = dwc2_readl(dwc2, GRSTCTL);
	greset |= GRSTCTL_CSFTRST;
	dwc2_writel(dwc2, greset, GRSTCTL);

	ret = dwc2_wait_bit_clear(dwc2, GRSTCTL, GRSTCTL_CSFTRST, 10000);
	if (ret) {
		dwc2_warn(dwc2, "%s: Timeout! Waiting for Core Soft Reset\n",
				__func__);
		return ret;
	}

	if (wait_for_host_mode)
		dwc2_wait_for_mode(dwc2, wait_for_host_mode);

	return 0;
}

/*
 * This function initializes the DWC2 controller registers and
 * prepares the core for device mode or host mode operation.
 *
 * @param regs Programming view of the DWC2 controller
 */
void dwc2_core_init(struct dwc2 *dwc2)
{
	uint32_t otgctl = 0;
	uint32_t usbcfg = 0;
	int retval;

	dwc2_dbg(dwc2, "%s(%p)\n", __func__, dwc2);

	/* Common Initialization */
	usbcfg = dwc2_readl(dwc2, GUSBCFG);

	/* Set ULPI External VBUS bit if needed */
	usbcfg &= ~GUSBCFG_ULPI_EXT_VBUS_DRV;
	if (dwc2->params.phy_ulpi_ext_vbus)
		usbcfg |= GUSBCFG_ULPI_EXT_VBUS_DRV;

	/* Set external TS Dline pulsing bit if needed */
	usbcfg &= ~GUSBCFG_TERMSELDLPULSE;
	if (dwc2->params.ts_dline)
		usbcfg |= GUSBCFG_TERMSELDLPULSE;

	dwc2_writel(dwc2, usbcfg, GUSBCFG);

	/* Reset the Controller */
	dwc2_core_reset(dwc2);

	/*
	 * This programming sequence needs to happen in FS mode before
	 * any other programming occurs
	 */
	retval = dwc2_phy_init(dwc2, true);
	if (retval)
		return;

	/* Program the GAHBCFG Register */
	retval = dwc2_gahbcfg_init(dwc2);
	if (retval)
		return;

	/* Program the GUSBCFG register */
	dwc2_gusbcfg_init(dwc2);

	/* Program the GOTGCTL register */
	otgctl = dwc2_readl(dwc2, GOTGCTL);
	otgctl &= ~GOTGCTL_OTGVER;
	dwc2_writel(dwc2, otgctl, GOTGCTL);

	if (dwc2_is_host_mode(dwc2))
		dwc2_dbg(dwc2, "Host Mode\n");
	else
		dwc2_dbg(dwc2, "Device Mode\n");
}
