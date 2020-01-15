// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2012 Oleksandr Tymoshenko <gonzo@freebsd.org>
 * Copyright (C) 2014 Marek Vasut <marex@denx.de>
 */

#include <common.h>
#include <usb/usb.h>
#include <usb/usbroothubdes.h>
#include <malloc.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <linux/iopoll.h>
#include <dma.h>

#include "dwc2.h"

/* Use only HC channel 0. */
#define DWC2_HC_CHANNEL			0

#define DWC2_STATUS_BUF_SIZE		64
#define DWC2_DATA_BUF_SIZE		(16 * 1024)

#define MAX_DEVICE			16
#define MAX_ENDPOINT			16

struct dwc2_priv {
	struct device_d *dev;
	struct usb_host host;
	uint8_t *dmabuf;

	u8 in_data_toggle[MAX_DEVICE][MAX_ENDPOINT];
	u8 out_data_toggle[MAX_DEVICE][MAX_ENDPOINT];
	struct dwc2_core_regs *regs;
	int root_hub_devnum;
	bool ext_vbus;
	/*
	 * The hnp/srp capability must be disabled if the platform
	 * does't support hnp/srp. Otherwise the force mode can't work.
	 */
	bool hnp_srp_disable;
	bool oc_disable;
};

/*
 * Initializes the FSLSPClkSel field of the HCFG register
 * depending on the PHY type.
 */
static void init_fslspclksel(struct dwc2_priv *priv)
{
	struct dwc2_core_regs *regs = priv->regs;
	uint32_t phyclk;

	phyclk = DWC2_HCFG_FSLSPCLKSEL_48_MHZ;	/* Full speed PHY */

	clrsetbits_le32(&regs->host_regs.hcfg,
			DWC2_HCFG_FSLSPCLKSEL_MASK,
			phyclk << DWC2_HCFG_FSLSPCLKSEL_OFFSET);
}

/*
 * Flush a Tx FIFO.
 *
 * @param regs Programming view of DWC_otg controller.
 * @param num Tx FIFO to flush.
 */
static void dwc_otg_flush_tx_fifo(struct dwc2_priv *priv, const int num)
{
	struct dwc2_core_regs *regs = priv->regs;
	struct device_d *dev = priv->dev;
	int ret;
	uint32_t val;

	writel(DWC2_GRSTCTL_TXFFLSH | (num << DWC2_GRSTCTL_TXFNUM_OFFSET),
	       &regs->grstctl);
	ret = readl_poll_timeout(&regs->grstctl, val, !(val & DWC2_GRSTCTL_TXFFLSH),
				 1000000);
	if (ret)
		dev_err(dev, "%s: Timeout!\n", __func__);

	/* Wait for 3 PHY Clocks */
	udelay(1);
}

/*
 * Flush Rx FIFO.
 *
 * @param regs Programming view of DWC_otg controller.
 */
static void dwc_otg_flush_rx_fifo(struct dwc2_priv *priv)
{
	struct dwc2_core_regs *regs = priv->regs;
	struct device_d *dev = priv->dev;
	int ret;
	uint32_t val;

	writel(DWC2_GRSTCTL_RXFFLSH, &regs->grstctl);
	ret = readl_poll_timeout(&regs->grstctl, val, !(val & DWC2_GRSTCTL_RXFFLSH),
				 1000000);
	if (ret)
		dev_err(dev, "%s: Timeout!\n", __func__);

	/* Wait for 3 PHY Clocks */
	udelay(1);
}

/*
 * Do core a soft reset of the core.  Be careful with this because it
 * resets all the internal state machines of the core.
 */
static void dwc_otg_core_reset(struct dwc2_priv *priv)
{
	struct dwc2_core_regs *regs = priv->regs;
	struct device_d *dev = priv->dev;
	uint32_t val;
	int ret;

	/* Wait for AHB master IDLE state. */
	ret = readl_poll_timeout(&regs->grstctl, val, val & DWC2_GRSTCTL_AHBIDLE,
				 1000000);
	if (ret)
		dev_err(dev, "%s: Timeout!\n", __func__);

	/* Core Soft Reset */
	writel(DWC2_GRSTCTL_CSFTRST, &regs->grstctl);
	ret = readl_poll_timeout(&regs->grstctl, val, !(val & DWC2_GRSTCTL_CSFTRST),
				 1000000);
	if (ret)
		dev_err(dev, "%s: Timeout!\n", __func__);

	/*
	 * Wait for core to come out of reset.
	 * NOTE: This long sleep is _very_ important, otherwise the core will
	 *       not stay in host mode after a connector ID change!
	 */
	mdelay(100);
}

/*
 * This function initializes the DWC_otg controller registers for
 * host mode.
 *
 * This function flushes the Tx and Rx FIFOs and it flushes any entries in the
 * request queues. Host channels are reset to ensure that they are ready for
 * performing transfers.
 *
 * @param dev USB Device (NULL if driver model is not being used)
 * @param regs Programming view of DWC_otg controller
 *
 */
static void dwc_otg_core_host_init(struct dwc2_priv *priv)
{
	struct dwc2_core_regs *regs = priv->regs;
	struct device_d *dev = priv->dev;
	uint32_t nptxfifosize = 0;
	uint32_t ptxfifosize = 0;
	uint32_t hprt0 = 0;
	uint32_t val;
	int i, ret, num_channels;

	/* Restart the Phy Clock */
	writel(0, &regs->pcgcctl);

	/* Initialize Host Configuration Register */
	init_fslspclksel(priv);

	/* Configure data FIFO sizes */
	if (readl(&regs->ghwcfg2) & DWC2_HWCFG2_DYNAMIC_FIFO) {
		/* Rx FIFO */
		writel(CONFIG_DWC2_HOST_RX_FIFO_SIZE, &regs->grxfsiz);

		/* Non-periodic Tx FIFO */
		nptxfifosize |= CONFIG_DWC2_HOST_NPERIO_TX_FIFO_SIZE <<
				DWC2_FIFOSIZE_DEPTH_OFFSET;
		nptxfifosize |= CONFIG_DWC2_HOST_RX_FIFO_SIZE <<
				DWC2_FIFOSIZE_STARTADDR_OFFSET;
		writel(nptxfifosize, &regs->gnptxfsiz);

		/* Periodic Tx FIFO */
		ptxfifosize |= CONFIG_DWC2_HOST_PERIO_TX_FIFO_SIZE <<
				DWC2_FIFOSIZE_DEPTH_OFFSET;
		ptxfifosize |= (CONFIG_DWC2_HOST_RX_FIFO_SIZE +
				CONFIG_DWC2_HOST_NPERIO_TX_FIFO_SIZE) <<
				DWC2_FIFOSIZE_STARTADDR_OFFSET;
		writel(ptxfifosize, &regs->hptxfsiz);
	}

	/* Clear Host Set HNP Enable in the OTG Control Register */
	clrbits_le32(&regs->gotgctl, DWC2_GOTGCTL_HSTSETHNPEN);

	/* Make sure the FIFOs are flushed. */
	dwc_otg_flush_tx_fifo(priv, 0x10);	/* All Tx FIFOs */
	dwc_otg_flush_rx_fifo(priv);

	/* Flush out any leftover queued requests. */
	num_channels = readl(&regs->ghwcfg2);
	num_channels &= DWC2_HWCFG2_NUM_HOST_CHAN_MASK;
	num_channels >>= DWC2_HWCFG2_NUM_HOST_CHAN_OFFSET;
	num_channels += 1;

	for (i = 0; i < num_channels; i++)
		clrsetbits_le32(&regs->hc_regs[i].hcchar,
				DWC2_HCCHAR_CHEN | DWC2_HCCHAR_EPDIR,
				DWC2_HCCHAR_CHDIS);

	/* Halt all channels to put them into a known state. */
	for (i = 0; i < num_channels; i++) {
		clrsetbits_le32(&regs->hc_regs[i].hcchar,
				DWC2_HCCHAR_EPDIR,
				DWC2_HCCHAR_CHEN | DWC2_HCCHAR_CHDIS);
		ret = readl_poll_timeout(&regs->hc_regs[i].hcchar, val,
				 !(val & DWC2_HCCHAR_CHEN),
				 1000000);
		if (ret)
			dev_err(dev, "%s: Timeout!\n", __func__);
	}

	/* Turn on the vbus power. */
	if (readl(&regs->gintsts) & DWC2_GINTSTS_CURMODE_HOST) {
		hprt0 = readl(&regs->hprt0);
		hprt0 &= ~(DWC2_HPRT0_PRTENA | DWC2_HPRT0_PRTCONNDET);
		hprt0 &= ~(DWC2_HPRT0_PRTENCHNG | DWC2_HPRT0_PRTOVRCURRCHNG);
		if (!(hprt0 & DWC2_HPRT0_PRTPWR)) {
			hprt0 |= DWC2_HPRT0_PRTPWR;
			writel(hprt0, &regs->hprt0);
		}
	}
}

/*
 * This function initializes the DWC_otg controller registers and
 * prepares the core for device mode or host mode operation.
 *
 * @param regs Programming view of the DWC_otg controller
 */
static void dwc_otg_core_init(struct dwc2_priv *priv)
{
	struct dwc2_core_regs *regs = priv->regs;
	uint32_t ahbcfg = 0;
	uint32_t usbcfg = 0;
	uint8_t brst_sz = 32;

	/* Common Initialization */
	usbcfg = readl(&regs->gusbcfg);

	/* Program the ULPI External VBUS bit if needed */
	if (priv->ext_vbus) {
		usbcfg |= DWC2_GUSBCFG_ULPI_EXT_VBUS_DRV;
		if (!priv->oc_disable) {
			usbcfg |= DWC2_GUSBCFG_ULPI_INT_VBUS_INDICATOR |
				  DWC2_GUSBCFG_INDICATOR_PASSTHROUGH;
		}
	} else {
		usbcfg &= ~DWC2_GUSBCFG_ULPI_EXT_VBUS_DRV;
	}

	/* Set external TS Dline pulsing */
	usbcfg &= ~DWC2_GUSBCFG_TERM_SEL_DL_PULSE;
	writel(usbcfg, &regs->gusbcfg);

	/* Reset the Controller */
	dwc_otg_core_reset(priv);

	/* High speed PHY. */

	/*
	 * HS PHY parameters. These parameters are preserved during
	 * soft reset so only program the first time. Do a soft reset
	 * immediately after setting phyif.
	 */
	usbcfg &= ~(DWC2_GUSBCFG_ULPI_UTMI_SEL | DWC2_GUSBCFG_PHYIF);
	usbcfg |= CONFIG_DWC2_PHY_TYPE << DWC2_GUSBCFG_ULPI_UTMI_SEL_OFFSET;

	if (usbcfg & DWC2_GUSBCFG_ULPI_UTMI_SEL) /* ULPI interface */
		usbcfg &= ~DWC2_GUSBCFG_DDRSEL;

	writel(usbcfg, &regs->gusbcfg);

	/* Reset after setting the PHY parameters */
	dwc_otg_core_reset(priv);

	usbcfg = readl(&regs->gusbcfg);
	usbcfg &= ~(DWC2_GUSBCFG_ULPI_FSLS | DWC2_GUSBCFG_ULPI_CLK_SUS_M);

	if (priv->hnp_srp_disable)
		usbcfg |= DWC2_GUSBCFG_FORCEHOSTMODE;

	writel(usbcfg, &regs->gusbcfg);

	/* Program the GAHBCFG Register. */
	switch (readl(&regs->ghwcfg2) & DWC2_HWCFG2_ARCHITECTURE_MASK) {
	case DWC2_HWCFG2_ARCHITECTURE_SLAVE_ONLY:
		break;
	case DWC2_HWCFG2_ARCHITECTURE_EXT_DMA:
		while (brst_sz > 1) {
			ahbcfg |= ahbcfg + (1 << DWC2_GAHBCFG_HBURSTLEN_OFFSET);
			ahbcfg &= DWC2_GAHBCFG_HBURSTLEN_MASK;
			brst_sz >>= 1;
		}

		ahbcfg |= DWC2_GAHBCFG_DMAENABLE;
		break;

	case DWC2_HWCFG2_ARCHITECTURE_INT_DMA:
		ahbcfg |= DWC2_GAHBCFG_HBURSTLEN_INCR4;
		ahbcfg |= DWC2_GAHBCFG_DMAENABLE;
		break;
	}

	writel(ahbcfg, &regs->gahbcfg);

	/* Program the capabilities in GUSBCFG Register */
	usbcfg = 0;

	if (!priv->hnp_srp_disable)
		usbcfg |= DWC2_GUSBCFG_HNPCAP | DWC2_GUSBCFG_SRPCAP;

	setbits_le32(&regs->gusbcfg, usbcfg);
}

/*
 * Prepares a host channel for transferring packets to/from a specific
 * endpoint. The HCCHARn register is set up with the characteristics specified
 * in _hc. Host channel interrupts that may need to be serviced while this
 * transfer is in progress are enabled.
 *
 * @param regs Programming view of DWC_otg controller
 * @param hc Information needed to initialize the host channel
 */
static void dwc_otg_hc_init(struct dwc2_core_regs *regs, uint8_t hc_num,
		struct usb_device *dev, uint8_t dev_addr, uint8_t ep_num,
		uint8_t ep_is_in, uint8_t ep_type, uint16_t max_packet)
{
	struct dwc2_hc_regs *hc_regs = &regs->hc_regs[hc_num];
	uint32_t hcchar = (dev_addr << DWC2_HCCHAR_DEVADDR_OFFSET) |
			  (ep_num << DWC2_HCCHAR_EPNUM_OFFSET) |
			  (ep_is_in << DWC2_HCCHAR_EPDIR_OFFSET) |
			  (ep_type << DWC2_HCCHAR_EPTYPE_OFFSET) |
			  (max_packet << DWC2_HCCHAR_MPS_OFFSET);

	if (dev->speed == USB_SPEED_LOW)
		hcchar |= DWC2_HCCHAR_LSPDDEV;

	/*
	 * Program the HCCHARn register with the endpoint characteristics
	 * for the current transfer.
	 */
	writel(hcchar, &hc_regs->hcchar);

	/* Program the HCSPLIT register, default to no SPLIT */
	writel(0, &hc_regs->hcsplt);
}

static void dwc_otg_hc_init_split(struct dwc2_hc_regs *hc_regs,
				  uint8_t hub_devnum, uint8_t hub_port)
{
	uint32_t hcsplt = 0;

	hcsplt = DWC2_HCSPLT_SPLTENA;
	hcsplt |= hub_devnum << DWC2_HCSPLT_HUBADDR_OFFSET;
	hcsplt |= hub_port << DWC2_HCSPLT_PRTADDR_OFFSET;

	/* Program the HCSPLIT register for SPLITs */
	writel(hcsplt, &hc_regs->hcsplt);
}

/*
 * DWC2 to USB API interface
 */
/* Direction: In ; Request: Status */
static int dwc_otg_submit_rh_msg_in_status(struct dwc2_core_regs *regs,
					   struct usb_device *dev, void *buffer,
					   int txlen, struct devrequest *cmd)
{
	uint32_t hprt0 = 0;
	uint32_t port_status = 0;
	uint32_t port_change = 0;
	int len = 0;
	int stat = 0;

	switch (cmd->requesttype & ~USB_DIR_IN) {
	case 0:
		*(uint16_t *)buffer = cpu_to_le16(1);
		len = 2;
		break;
	case USB_RECIP_INTERFACE:
	case USB_RECIP_ENDPOINT:
		*(uint16_t *)buffer = cpu_to_le16(0);
		len = 2;
		break;
	case USB_TYPE_CLASS:
		*(uint32_t *)buffer = cpu_to_le32(0);
		len = 4;
		break;
	case USB_RECIP_OTHER | USB_TYPE_CLASS:
		hprt0 = readl(&regs->hprt0);
		if (hprt0 & DWC2_HPRT0_PRTCONNSTS)
			port_status |= USB_PORT_STAT_CONNECTION;
		if (hprt0 & DWC2_HPRT0_PRTENA)
			port_status |= USB_PORT_STAT_ENABLE;
		if (hprt0 & DWC2_HPRT0_PRTSUSP)
			port_status |= USB_PORT_STAT_SUSPEND;
		if (hprt0 & DWC2_HPRT0_PRTOVRCURRACT)
			port_status |= USB_PORT_STAT_OVERCURRENT;
		if (hprt0 & DWC2_HPRT0_PRTRST)
			port_status |= USB_PORT_STAT_RESET;
		if (hprt0 & DWC2_HPRT0_PRTPWR)
			port_status |= USB_PORT_STAT_POWER;

		if ((hprt0 & DWC2_HPRT0_PRTSPD_MASK) == DWC2_HPRT0_PRTSPD_LOW)
			port_status |= USB_PORT_STAT_LOW_SPEED;
		else if ((hprt0 & DWC2_HPRT0_PRTSPD_MASK) ==
			 DWC2_HPRT0_PRTSPD_HIGH)
			port_status |= USB_PORT_STAT_HIGH_SPEED;

		if (hprt0 & DWC2_HPRT0_PRTENCHNG)
			port_change |= USB_PORT_STAT_C_ENABLE;
		if (hprt0 & DWC2_HPRT0_PRTCONNDET)
			port_change |= USB_PORT_STAT_C_CONNECTION;
		if (hprt0 & DWC2_HPRT0_PRTOVRCURRCHNG)
			port_change |= USB_PORT_STAT_C_OVERCURRENT;

		*(uint32_t *)buffer = cpu_to_le32(port_status |
					(port_change << 16));
		len = 4;
		break;
	default:
		pr_err("%s: unsupported root hub command\n", __func__);
		stat = USB_ST_STALLED;
	}

	dev->act_len = min(len, txlen);
	dev->status = stat;

	return stat;
}

/* Direction: In ; Request: Descriptor */
static int dwc_otg_submit_rh_msg_in_descriptor(struct usb_device *dev,
					       void *buffer, int txlen,
					       struct devrequest *cmd)
{
	unsigned char data[32];
	uint32_t dsc;
	int len = 0;
	int stat = 0;
	uint16_t wValue = cpu_to_le16(cmd->value);
	uint16_t wLength = cpu_to_le16(cmd->length);

	switch (cmd->requesttype & ~USB_DIR_IN) {
	case 0:
		switch (wValue & 0xff00) {
		case 0x0100:	/* device descriptor */
			len = min3(txlen, (int)sizeof(root_hub_dev_des), (int)wLength);
			memcpy(buffer, root_hub_dev_des, len);
			break;
		case 0x0200:	/* configuration descriptor */
			len = min3(txlen, (int)sizeof(root_hub_config_des), (int)wLength);
			memcpy(buffer, root_hub_config_des, len);
			break;
		case 0x0300:	/* string descriptors */
			switch (wValue & 0xff) {
			case 0x00:
				len = min3(txlen, (int)sizeof(root_hub_str_index0),
					   (int)wLength);
				memcpy(buffer, root_hub_str_index0, len);
				break;
			case 0x01:
				len = min3(txlen, (int)sizeof(root_hub_str_index1),
					   (int)wLength);
				memcpy(buffer, root_hub_str_index1, len);
				break;
			}
			break;
		default:
			stat = USB_ST_STALLED;
		}
		break;

	case USB_TYPE_CLASS:
		/* Root port config, set 1 port and nothing else. */
		dsc = 0x00000001;

		data[0] = 9;		/* min length; */
		data[1] = 0x29;
		data[2] = dsc & RH_A_NDP;
		data[3] = 0;
		if (dsc & RH_A_PSM)
			data[3] |= 0x1;
		if (dsc & RH_A_NOCP)
			data[3] |= 0x10;
		else if (dsc & RH_A_OCPM)
			data[3] |= 0x8;

		/* corresponds to data[4-7] */
		data[5] = (dsc & RH_A_POTPGT) >> 24;
		data[7] = dsc & RH_B_DR;
		if (data[2] < 7) {
			data[8] = 0xff;
		} else {
			data[0] += 2;
			data[8] = (dsc & RH_B_DR) >> 8;
			data[9] = 0xff;
			data[10] = data[9];
		}

		len = min3(txlen, (int)data[0], (int)wLength);
		memcpy(buffer, data, len);
		break;
	default:
		pr_err("%s: unsupported root hub command\n", __func__);
		stat = USB_ST_STALLED;
	}

	dev->act_len = min(len, txlen);
	dev->status = stat;

	return stat;
}

/* Direction: In ; Request: Configuration */
static int dwc_otg_submit_rh_msg_in_configuration(struct usb_device *dev,
						  void *buffer, int txlen,
						  struct devrequest *cmd)
{
	int len = 0;
	int stat = 0;

	switch (cmd->requesttype & ~USB_DIR_IN) {
	case 0:
		*(uint8_t *)buffer = 0x01;
		len = 1;
		break;
	default:
		pr_err("%s: unsupported root hub command\n", __func__);
		stat = USB_ST_STALLED;
	}

	dev->act_len = min(len, txlen);
	dev->status = stat;

	return stat;
}

/* Direction: In */
static int dwc_otg_submit_rh_msg_in(struct dwc2_priv *priv,
				    struct usb_device *dev, void *buffer,
				    int txlen, struct devrequest *cmd)
{
	switch (cmd->request) {
	case USB_REQ_GET_STATUS:
		return dwc_otg_submit_rh_msg_in_status(priv->regs, dev, buffer,
						       txlen, cmd);
	case USB_REQ_GET_DESCRIPTOR:
		return dwc_otg_submit_rh_msg_in_descriptor(dev, buffer,
							   txlen, cmd);
	case USB_REQ_GET_CONFIGURATION:
		return dwc_otg_submit_rh_msg_in_configuration(dev, buffer,
							      txlen, cmd);
	default:
		pr_err("%s: unsupported root hub command\n", __func__);
		return USB_ST_STALLED;
	}
}

/* Direction: Out */
static int dwc_otg_submit_rh_msg_out(struct dwc2_priv *priv,
				     struct usb_device *dev,
				     void *buffer, int txlen,
				     struct devrequest *cmd)
{
	struct dwc2_core_regs *regs = priv->regs;
	int len = 0;
	int stat = 0;
	uint16_t bmrtype_breq = cmd->requesttype | (cmd->request << 8);
	uint16_t wValue = cpu_to_le16(cmd->value);

	switch (bmrtype_breq & ~USB_DIR_IN) {
	case (USB_REQ_CLEAR_FEATURE << 8) | USB_RECIP_ENDPOINT:
	case (USB_REQ_CLEAR_FEATURE << 8) | USB_TYPE_CLASS:
		break;

	case (USB_REQ_CLEAR_FEATURE << 8) | USB_RECIP_OTHER | USB_TYPE_CLASS:
		switch (wValue) {
		case USB_PORT_FEAT_C_CONNECTION:
			setbits_le32(&regs->hprt0, DWC2_HPRT0_PRTCONNDET);
			break;
		}
		break;

	case (USB_REQ_SET_FEATURE << 8) | USB_RECIP_OTHER | USB_TYPE_CLASS:
		switch (wValue) {
		case USB_PORT_FEAT_SUSPEND:
			break;

		case USB_PORT_FEAT_RESET:
			clrsetbits_le32(&regs->hprt0, DWC2_HPRT0_PRTENA |
					DWC2_HPRT0_PRTCONNDET |
					DWC2_HPRT0_PRTENCHNG |
					DWC2_HPRT0_PRTOVRCURRCHNG,
					DWC2_HPRT0_PRTRST);
			mdelay(50);
			clrbits_le32(&regs->hprt0, DWC2_HPRT0_PRTRST);
			break;

		case USB_PORT_FEAT_POWER:
			clrsetbits_le32(&regs->hprt0, DWC2_HPRT0_PRTENA |
					DWC2_HPRT0_PRTCONNDET |
					DWC2_HPRT0_PRTENCHNG |
					DWC2_HPRT0_PRTOVRCURRCHNG,
					DWC2_HPRT0_PRTRST);
			break;

		case USB_PORT_FEAT_ENABLE:
			break;
		}
		break;
	case (USB_REQ_SET_ADDRESS << 8):
		priv->root_hub_devnum = wValue;
		break;
	case (USB_REQ_SET_CONFIGURATION << 8):
		break;
	default:
		pr_err("%s: unsupported root hub command\n", __func__);
		stat = USB_ST_STALLED;
	}

	len = min(len, txlen);

	dev->act_len = len;
	dev->status = stat;

	return stat;
}

static int dwc_otg_submit_rh_msg(struct dwc2_priv *priv, struct usb_device *dev,
				 unsigned long pipe, void *buffer, int txlen,
				 struct devrequest *cmd)
{
	int stat = 0;

	if (usb_pipeint(pipe)) {
		pr_err("Root-Hub submit IRQ: NOT implemented\n");
		return 0;
	}

	if (cmd->requesttype & USB_DIR_IN)
		stat = dwc_otg_submit_rh_msg_in(priv, dev, buffer, txlen, cmd);
	else
		stat = dwc_otg_submit_rh_msg_out(priv, dev, buffer, txlen, cmd);

	mdelay(1);

	return stat;
}

static int wait_for_chhltd(struct dwc2_priv *priv, uint32_t *sub,
			   u8 *toggle, int timeout_ms)
{
	struct dwc2_core_regs *regs = priv->regs;
	struct dwc2_hc_regs *hc_regs = &regs->hc_regs[DWC2_HC_CHANNEL];
	struct device_d *dev = priv->dev;
	int ret;
	uint32_t hcint, hctsiz;
	uint32_t val;
	int timeout_us = timeout_ms * 1000;

	ret = readl_poll_timeout(&hc_regs->hcint, val,
				 val & DWC2_HCINT_CHHLTD, timeout_us);
	if (ret) {
		clrsetbits_le32(&hc_regs->hcchar, 0, DWC2_HCCHAR_CHDIS);
		readl_poll_timeout(&hc_regs->hcint, val,
					 val & DWC2_HCINT_CHHLTD, 10000);

		return ret;
	}

	hcint = readl(&hc_regs->hcint);
	hctsiz = readl(&hc_regs->hctsiz);
	*sub = (hctsiz & DWC2_HCTSIZ_XFERSIZE_MASK) >>
		DWC2_HCTSIZ_XFERSIZE_OFFSET;
	*toggle = (hctsiz & DWC2_HCTSIZ_PID_MASK) >> DWC2_HCTSIZ_PID_OFFSET;

	dev_dbg(dev, "%s: HCINT=%08x sub=%u toggle=%d\n", __func__, hcint, *sub,
	      *toggle);

	if (hcint & DWC2_HCINT_XFERCOMP)
		return 0;

	if (hcint & (DWC2_HCINT_NAK | DWC2_HCINT_FRMOVRUN))
		return -EAGAIN;

	dev_dbg(dev, "%s: Error (HCINT=%08x)\n", __func__, hcint);

	return -EINVAL;
}

static int dwc2_eptype[] = {
	DWC2_HCCHAR_EPTYPE_ISOC,
	DWC2_HCCHAR_EPTYPE_INTR,
	DWC2_HCCHAR_EPTYPE_CONTROL,
	DWC2_HCCHAR_EPTYPE_BULK,
};

static int transfer_chunk(struct dwc2_priv *priv, u8 *pid, int in, void *buffer,
			  int num_packets, int xfer_len, int *actual_len,
			  int odd_frame, int timeout_ms)
{
	struct dwc2_core_regs *regs = priv->regs;
	struct dwc2_hc_regs *hc_regs = &regs->hc_regs[DWC2_HC_CHANNEL];
	int ret = 0;
	uint32_t sub = 0;
	enum dma_data_direction dir;
	dma_addr_t dma = 0;

	dev_dbg(priv->dev, "%s: chunk: pid %d xfer_len %u pkts %u\n",
		__func__, *pid, xfer_len, num_packets);

	writel((xfer_len << DWC2_HCTSIZ_XFERSIZE_OFFSET) |
	       (num_packets << DWC2_HCTSIZ_PKTCNT_OFFSET) |
	       (*pid << DWC2_HCTSIZ_PID_OFFSET),
	       &hc_regs->hctsiz);

	if (xfer_len) {
		if (in) {
			dir = DMA_FROM_DEVICE;
		} else {
			memcpy(priv->dmabuf, buffer, xfer_len);
			dir = DMA_TO_DEVICE;
		}
		dma = dma_map_single(priv->dev, priv->dmabuf, xfer_len, dir);
	}

	writel(dma, &hc_regs->hcdma);

	/* Clear old interrupt conditions for this host channel. */
	writel(0x3fff, &hc_regs->hcint);

	/* Set host channel enable after all other setup is complete. */
	clrsetbits_le32(&hc_regs->hcchar, DWC2_HCCHAR_MULTICNT_MASK |
			DWC2_HCCHAR_CHEN | DWC2_HCCHAR_CHDIS |
			DWC2_HCCHAR_ODDFRM,
			(1 << DWC2_HCCHAR_MULTICNT_OFFSET) |
			(odd_frame << DWC2_HCCHAR_ODDFRM_OFFSET) |
			DWC2_HCCHAR_CHEN);

	ret = wait_for_chhltd(priv, &sub, pid, timeout_ms);

	if (xfer_len)
		dma_unmap_single(priv->dev, dma, xfer_len, dir);

	if (in) {
		xfer_len -= sub;

		memcpy(buffer, priv->dmabuf, xfer_len);
	}

	if (!ret)
		*actual_len = xfer_len;

	return ret;
}

static int usb_find_usb2_hub_address_port(struct usb_device *udev,
                               uint8_t *hub_address, uint8_t *hub_port)
{
	/* Find out the nearest parent which is high speed */
	while (udev->parent->parent) {
		if (udev->parent->speed != USB_SPEED_HIGH) {
			udev = udev->parent;
		} else {
			*hub_address = udev->parent->devnum;
			*hub_port = udev->portnr;
			return 0;
		}
	}

	return -EINVAL;
}

static int chunk_msg(struct dwc2_priv *priv, struct usb_device *dev,
	      unsigned long pipe, u8 *pid, int in, void *buffer, int len,
	      int timeout_ms)
{
	struct dwc2_core_regs *regs = priv->regs;
	struct dwc2_hc_regs *hc_regs = &regs->hc_regs[DWC2_HC_CHANNEL];
	struct dwc2_host_regs *host_regs = &regs->host_regs;
	int devnum = usb_pipedevice(pipe);
	int ep = usb_pipeendpoint(pipe);
	int max = usb_maxpacket(dev, pipe);
	int eptype = dwc2_eptype[usb_pipetype(pipe)];
	int done = 0;
	int ret = 0;
	int do_split = 0;
	int complete_split = 0;
	uint32_t xfer_len;
	uint32_t num_packets;
	int stop_transfer = 0;
	uint32_t max_xfer_len;
	int ssplit_frame_num = 0;

	dev_dbg(priv->dev, "%s: msg: pipe %lx pid %d in %d len %d\n",
		__func__, pipe, *pid, in, len);

	max_xfer_len = CONFIG_DWC2_MAX_PACKET_COUNT * max;
	if (max_xfer_len > CONFIG_DWC2_MAX_TRANSFER_SIZE)
		max_xfer_len = CONFIG_DWC2_MAX_TRANSFER_SIZE;
	if (max_xfer_len > DWC2_DATA_BUF_SIZE)
		max_xfer_len = DWC2_DATA_BUF_SIZE;

	/* Make sure that max_xfer_len is a multiple of max packet size. */
	num_packets = max_xfer_len / max;
	max_xfer_len = num_packets * max;

	/* Initialize channel */
	dwc_otg_hc_init(regs, DWC2_HC_CHANNEL, dev, devnum, ep, in,
			eptype, max);

	/* Check if the target is a FS/LS device behind a HS hub */
	if (dev->speed != USB_SPEED_HIGH) {
		uint8_t hub_addr;
		uint8_t hub_port;
		uint32_t hprt0 = readl(&regs->hprt0);

		if ((hprt0 & DWC2_HPRT0_PRTSPD_MASK) == DWC2_HPRT0_PRTSPD_HIGH) {
			ret = usb_find_usb2_hub_address_port(dev, &hub_addr,
						       &hub_port);
			if (ret)
				return ret;
			dwc_otg_hc_init_split(hc_regs, hub_addr, hub_port);

			do_split = 1;
			num_packets = 1;
			max_xfer_len = max;
		}
	}

	do {
		int actual_len = 0;
		uint32_t hcint;
		int odd_frame = 0;
		xfer_len = len - done;

		if (xfer_len > max_xfer_len)
			xfer_len = max_xfer_len;
		else if (xfer_len > max)
			num_packets = (xfer_len + max - 1) / max;
		else
			num_packets = 1;

		if (complete_split)
			setbits_le32(&hc_regs->hcsplt, DWC2_HCSPLT_COMPSPLT);
		else if (do_split)
			clrbits_le32(&hc_regs->hcsplt, DWC2_HCSPLT_COMPSPLT);

		if (eptype == DWC2_HCCHAR_EPTYPE_INTR) {
			int uframe_num = readl(&host_regs->hfnum);
			if (!(uframe_num & 0x1))
				odd_frame = 1;
		}

		ret = transfer_chunk(priv, pid, in, (char *)buffer + done,
				     num_packets, xfer_len, &actual_len,
				     odd_frame, timeout_ms);

		hcint = readl(&hc_regs->hcint);
		if (complete_split) {
			stop_transfer = 0;
			if (hcint & DWC2_HCINT_NYET) {
				int frame_num = DWC2_HFNUM_MAX_FRNUM &
						readl(&host_regs->hfnum);
				ret = 0;
				if (((frame_num - ssplit_frame_num) &
				    DWC2_HFNUM_MAX_FRNUM) > 4)
					ret = -EAGAIN;
			} else
				complete_split = 0;
		} else if (do_split) {
			if (hcint & DWC2_HCINT_ACK) {
				ssplit_frame_num = DWC2_HFNUM_MAX_FRNUM &
						   readl(&host_regs->hfnum);
				ret = 0;
				complete_split = 1;
			}
		}

		if (ret)
			break;

		if (actual_len < xfer_len)
			stop_transfer = 1;

		done += actual_len;

	/*
	 * Transactions are done when when either all data is transferred or
	 * there is a short transfer. In case of a SPLIT make sure the CSPLIT
	 * is executed.
	 */
	} while (((done < len) && !stop_transfer) || complete_split);

	writel(0, &hc_regs->hcintmsk);
	writel(0xFFFFFFFF, &hc_regs->hcint);

	dev->status = 0;
	dev->act_len = done;

	return ret;
}

#define to_dwc2(ptr) container_of(ptr, struct dwc2_priv, host)

static int dwc2_init_common(struct usb_host *host)
{
	struct dwc2_priv *priv = to_dwc2(host);
	struct dwc2_core_regs *regs = priv->regs;
	int i, j;

	priv->ext_vbus = 0;

	dwc_otg_core_init(priv);
	dwc_otg_core_host_init(priv);

	clrsetbits_le32(&regs->hprt0, DWC2_HPRT0_PRTENA |
			DWC2_HPRT0_PRTCONNDET | DWC2_HPRT0_PRTENCHNG |
			DWC2_HPRT0_PRTOVRCURRCHNG,
			DWC2_HPRT0_PRTRST);
	mdelay(50);
	clrbits_le32(&regs->hprt0, DWC2_HPRT0_PRTENA | DWC2_HPRT0_PRTCONNDET |
		     DWC2_HPRT0_PRTENCHNG | DWC2_HPRT0_PRTOVRCURRCHNG |
		     DWC2_HPRT0_PRTRST);

	for (i = 0; i < MAX_DEVICE; i++) {
		for (j = 0; j < MAX_ENDPOINT; j++) {
			priv->in_data_toggle[i][j] = DWC2_HC_PID_DATA0;
			priv->out_data_toggle[i][j] = DWC2_HC_PID_DATA0;
		}
	}

	return 0;
}

static void dwc2_uninit_common(struct dwc2_core_regs *regs)
{
	/* Put everything in reset. */
	clrsetbits_le32(&regs->hprt0, DWC2_HPRT0_PRTENA |
			DWC2_HPRT0_PRTCONNDET | DWC2_HPRT0_PRTENCHNG |
			DWC2_HPRT0_PRTOVRCURRCHNG,
			DWC2_HPRT0_PRTRST);
}

static int dwc2_submit_control_msg(struct usb_device *udev,
				   unsigned long pipe, void *buffer, int len,
				   struct devrequest *setup, int timeout_ms)
{
	struct usb_host *host = udev->host;
	struct dwc2_priv *priv = to_dwc2(host);
	int devnum = usb_pipedevice(pipe);
	int ret, act_len;
	u8 pid;
	/* For CONTROL endpoint pid should start with DATA1 */
	int status_direction;

	if (devnum == priv->root_hub_devnum) {
		udev->status = 0;
		udev->speed = USB_SPEED_HIGH;
		return dwc_otg_submit_rh_msg(priv, udev, pipe, buffer, len,
					     setup);
	}

	/* SETUP stage */
	pid = DWC2_HC_PID_SETUP;
	do {
		ret = chunk_msg(priv, udev, pipe, &pid, 0, setup, 8, timeout_ms);
	} while (ret == -EAGAIN);

	if (ret)
		return ret;

	/* DATA stage */
	act_len = 0;
	if (buffer) {
		pid = DWC2_HC_PID_DATA1;
		do {
			ret = chunk_msg(priv, udev, pipe, &pid, usb_pipein(pipe),
					buffer, len, timeout_ms);
			act_len += udev->act_len;
			buffer += udev->act_len;
			len -= udev->act_len;
		} while (ret == -EAGAIN);
		if (ret)
			return ret;
		status_direction = usb_pipeout(pipe);
	} else {
		/* No-data CONTROL always ends with an IN transaction */
		status_direction = 1;
	}

	/* STATUS stage */
	pid = DWC2_HC_PID_DATA1;
	do {
		ret = chunk_msg(priv, udev, pipe, &pid, status_direction,
				NULL, 0, timeout_ms);
	} while (ret == -EAGAIN);

	if (ret)
		return ret;

	udev->act_len = act_len;

	return 0;
}

static int dwc2_submit_bulk_msg(struct usb_device *udev, unsigned long pipe,
				void *buffer, int len, int timeout_ms)
{
	struct usb_host *host = udev->host;
	struct dwc2_priv *priv = to_dwc2(host);
	int devnum = usb_pipedevice(pipe);
	int ep = usb_pipeendpoint(pipe);
	u8* pid;

	if ((devnum >= MAX_DEVICE) || (devnum == priv->root_hub_devnum)) {
		udev->status = 0;
		return -EINVAL;
	}

	if (usb_pipein(pipe))
		pid = &priv->in_data_toggle[devnum][ep];
	else
		pid = &priv->out_data_toggle[devnum][ep];

	return chunk_msg(priv, udev, pipe, pid, usb_pipein(pipe), buffer, len,
			 timeout_ms);
}

static int dwc2_submit_int_msg(struct usb_device *udev, unsigned long pipe,
			       void *buffer, int len, int interval)
{
	uint64_t start;
	int ret;

	start = get_time_ns();

	while (1) {
		ret = dwc2_submit_bulk_msg(udev, pipe, buffer, len, 0);
		if (ret != -EAGAIN)
			return ret;
		if (is_timeout(start, USB_CNTL_TIMEOUT * MSECOND))
			return -ETIMEDOUT;
	}
}

static int dwc2_detect(struct device_d *dev)
{
	struct dwc2_priv *priv = dev->priv;

        return usb_host_detect(&priv->host);
}

static int dwc2_probe(struct device_d *dev)
{
	struct resource *iores;
	struct dwc2_priv *priv;
	struct usb_host *host;
	struct device_node *np = dev->device_node;
	int ret;
	uint32_t snpsid;

	priv = xzalloc(sizeof(*priv));

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	priv->regs = IOMEM(iores->start);
	priv->dev = dev;

	snpsid = readl(&priv->regs->gsnpsid);
	dev_info(dev, "Core Release: %x.%03x\n",
		 snpsid >> 12 & 0xf, snpsid & 0xfff);

	if ((snpsid & DWC2_SNPSID_DEVID_MASK) != DWC2_SNPSID_DEVID_VER_2xx &&
	    (snpsid & DWC2_SNPSID_DEVID_MASK) != DWC2_SNPSID_DEVID_VER_3xx) {
		dev_info(dev, "SNPSID invalid (not DWC2 OTG device): %08x\n",
			 snpsid);
		return -ENODEV;
	}

	priv->oc_disable = of_property_read_bool(np, "disable-over-current");
	priv->hnp_srp_disable = of_property_read_bool(np, "hnp-srp-disable");
	priv->dmabuf = dma_alloc(DWC2_DATA_BUF_SIZE);

	host = &priv->host;

	host->init = dwc2_init_common;
	host->submit_int_msg = dwc2_submit_int_msg;
	host->submit_control_msg = dwc2_submit_control_msg;
	host->submit_bulk_msg = dwc2_submit_bulk_msg;

	dev->priv = priv;
	dev->detect = dwc2_detect;

	ret = usb_register_host(host);
	if (ret)
		return ret;

	return 0;
}

static void dwc2_remove(struct device_d *dev)
{
	struct dwc2_priv *priv = dev->priv;

	dwc2_uninit_common(priv->regs);
}

static const struct of_device_id dwc2_dt_ids[] = {
	{ .compatible = "brcm,bcm2835-usb" },
	{ .compatible = "brcm,bcm2708-usb" },
	{ .compatible = "snps,dwc2" },
	{ /* sentinel */ }
};

static struct driver_d dwc2_driver = {
	.name = "dwc2",
	.probe = dwc2_probe,
	.remove = dwc2_remove,
	.of_compatible = DRV_OF_COMPAT(dwc2_dt_ids),
};
device_platform_driver(dwc2_driver);
