// SPDX-License-Identifier: GPL-2.0+
#include <dma.h>
#include "dwc2.h"

#define to_dwc2 host_to_dwc2

/* Use only HC channel 0. */
#define DWC2_HC_CHANNEL			0

static int dwc2_do_split(struct dwc2 *dwc2, struct usb_device *dev)
{
	uint32_t hprt0 = dwc2_readl(dwc2, HPRT0);
	uint32_t prtspd = (hprt0 & HPRT0_SPD_MASK) >> HPRT0_SPD_SHIFT;

	return prtspd == HPRT0_SPD_HIGH_SPEED && dev->speed != USB_SPEED_HIGH;
}

static void dwc2_host_hub_info(struct dwc2 *dwc2, struct usb_device *dev,
			       uint8_t *hub_addr, uint8_t *hub_port)
{
	*hub_addr = dev->devnum;
	*hub_port = dev->portnr;

	for (; dev->parent; dev = dev->parent) {
		if (dev->parent->descriptor->bDeviceClass == USB_CLASS_HUB) {
			*hub_addr = dev->parent->devnum;
			*hub_port = dev->parent->portnr;
			break;
		}
	}
}

static void dwc2_hc_init_split(struct dwc2 *dwc2, struct usb_device *dev,
			       uint8_t hc)
{
	uint8_t hub_addr, hub_port;
	uint32_t hcsplt = 0;

	dwc2_host_hub_info(dwc2, dev, &hub_addr, &hub_port);

	hcsplt = HCSPLT_SPLTENA;
	hcsplt |= hub_addr << HCSPLT_HUBADDR_SHIFT;
	hcsplt |= hub_port << HCSPLT_PRTADDR_SHIFT;

	/* Program the HCSPLIT register for SPLITs */
	dwc2_writel(dwc2, hcsplt, HCSPLT(hc));
}

static void dwc2_hc_enable_ints(struct dwc2 *dwc2, uint8_t hc)
{
	uint32_t intmsk;
	uint32_t hcintmsk = HCINTMSK_CHHLTD;

	dwc2_writel(dwc2, hcintmsk, HCINTMSK(hc));

	/* Enable the top level host channel interrupt */
	intmsk = dwc2_readl(dwc2, HAINTMSK);
	intmsk |= 1 << hc;
	dwc2_writel(dwc2, intmsk, HAINTMSK);

	/* Make sure host channel interrupts are enabled */
	intmsk = dwc2_readl(dwc2, GINTMSK);
	intmsk |= GINTSTS_HCHINT;
	dwc2_writel(dwc2, intmsk, GINTMSK);
}

/**
 * Prepares a host channel for transferring packets to/from a specific
 * endpoint. The HCCHARn register is set up with the characteristics specified
 * in _hc. Host channel interrupts that may need to be serviced while this
 * transfer is in progress are enabled.
 *
 * @param regs Programming view of DWC2 controller
 * @param hc Information needed to initialize the host channel
 */
static void dwc2_hc_init(struct dwc2 *dwc2, struct usb_device *dev, u8 hc,
			 unsigned long pipe, int is_in)
{
	int addr = usb_pipedevice(pipe);
	int endp = usb_pipeendpoint(pipe);
	int type = usb_pipetype(pipe);
	int mps = usb_maxpacket(dev, pipe);
	uint32_t hcchar = (addr << HCCHAR_DEVADDR_SHIFT) |
			  (endp << HCCHAR_EPNUM_SHIFT) |
			  (is_in ? HCCHAR_EPDIR : 0) |
			  (mps << HCCHAR_MPS_SHIFT);

	switch (type) {
	case PIPE_ISOCHRONOUS:
		hcchar |= DXEPCTL_EPTYPE_ISO;
		break;
	case PIPE_INTERRUPT:
		hcchar |= DXEPCTL_EPTYPE_INTERRUPT;
		break;
	case PIPE_CONTROL:
		hcchar |= DXEPCTL_EPTYPE_CONTROL;
		break;
	case PIPE_BULK:
		hcchar |= DXEPCTL_EPTYPE_BULK;
		break;
	}

	if (dev->speed == USB_SPEED_LOW)
		hcchar |= HCCHAR_LSPDDEV;

	/* Clear old interrupt conditions for this dwc2 channel */
	dwc2_writel(dwc2, ~HCINTMSK_RESERVED14_31, HCINT(hc));

	/* Enable channel interrupts required for this transfer */
	dwc2_hc_enable_ints(dwc2, hc);

	/*
	 * Program the HCCHARn register with the endpoint characteristics
	 * for the current transfer.
	 */
	dwc2_writel(dwc2, hcchar, HCCHAR(hc));

	/* Program the HCSPLIT register, default to no SPLIT */
	dwc2_writel(dwc2, 0, HCSPLT(hc));
}

static void dwc2_endpoint_reset(struct dwc2 *dwc2, int in, int devnum, int ep)
{
	if (in)
		dwc2->in_data_toggle[devnum][ep] = TSIZ_SC_MC_PID_DATA0;
	else
		dwc2->out_data_toggle[devnum][ep] = TSIZ_SC_MC_PID_DATA0;
}

static int wait_for_chhltd(struct dwc2 *dwc2, u8 hc, uint32_t *sub, u8 *tgl)
{
	int ret;
	uint32_t hcint, hctsiz, hcchar;

	ret = dwc2_wait_bit_set(dwc2, HCINT(hc), HCINTMSK_CHHLTD, 10000);
	if (ret) {
		hcchar = dwc2_readl(dwc2, HCCHAR(hc));
		dwc2_writel(dwc2, hcchar | HCCHAR_CHDIS, HCCHAR(hc));
		dwc2_wait_bit_set(dwc2, HCINT(hc), HCINTMSK_CHHLTD, 10000);
		return ret;
	}

	hcint = dwc2_readl(dwc2, HCINT(hc));

	if (hcint & HCINTMSK_AHBERR)
		dwc2_err(dwc2, "%s: AHB error during internal DMA access\n",
			   __func__);

	if (hcint & HCINTMSK_XFERCOMPL) {
		hctsiz = dwc2_readl(dwc2, HCTSIZ(hc));
		*sub = (hctsiz & TSIZ_XFERSIZE_MASK) >> TSIZ_XFERSIZE_SHIFT;
		*tgl = (hctsiz & TSIZ_SC_MC_PID_MASK) >> TSIZ_SC_MC_PID_SHIFT;

		dwc2_dbg(dwc2, "%s: HCINT=%08x sub=%u toggle=%d\n", __func__,
			 hcint, *sub, *tgl);
		return 0;
	}

	if (hcint & (HCINTMSK_NAK | HCINTMSK_FRMOVRUN))
		return -EAGAIN;

	dwc2_dbg(dwc2, "%s: Unknown channel status: (HCINT=%08x)\n", __func__,
		 hcint);
	return -EINVAL;
}

static int transfer_chunk(struct dwc2 *dwc2, u8 hc,
			  u8 *pid, int in, void *buffer, int num_packets,
			  int xfer_len, int *actual_len, int odd_frame)
{
	uint32_t hctsiz, hcchar, sub;
	dma_addr_t dma_addr;
	int ret = 0;

	dma_addr = dma_map_single(dwc2->dev, buffer, xfer_len,
				  in ? DMA_FROM_DEVICE : DMA_TO_DEVICE);

	if (dma_mapping_error(dwc2->dev, dma_addr)) {
		dwc2_err(dwc2, "Failed to map buffer@0x%p for dma\n", buffer);
		return -EFAULT;
	}

	dwc2_dbg(dwc2, "chunk: pid=%d xfer_len=%u pkts=%u dma_addr=%pad\n",
		 *pid, xfer_len, num_packets, (void *)dma_addr);

	dwc2_writel(dwc2, dma_addr, HCDMA(hc));

	hctsiz = (xfer_len << TSIZ_XFERSIZE_SHIFT)
		| (num_packets << TSIZ_PKTCNT_SHIFT)
		| (*pid << TSIZ_SC_MC_PID_SHIFT);

	dwc2_writel(dwc2, hctsiz, HCTSIZ(hc));

	/* Clear old interrupt conditions for this dwc2 channel. */
	dwc2_writel(dwc2, 0x3fff, HCINT(hc));

	/* Set dwc2 channel enable after all other setup is complete. */
	hcchar = dwc2_readl(dwc2, HCCHAR(hc));
	hcchar &= ~(HCCHAR_MULTICNT_MASK | HCCHAR_CHDIS);
	hcchar |= (1 << HCCHAR_MULTICNT_SHIFT) | HCCHAR_CHENA;
	if (odd_frame)
		hcchar |= HCCHAR_ODDFRM;
	else
		hcchar &= ~HCCHAR_ODDFRM;
	dwc2_writel(dwc2, hcchar, HCCHAR(hc));

	ret = wait_for_chhltd(dwc2, hc, &sub, pid);
	if (ret < 0)
		goto exit;

	if (in)
		xfer_len -= sub;
	*actual_len = xfer_len;

exit:
	dma_unmap_single(dwc2->dev, dma_addr, xfer_len,
				  in ? DMA_FROM_DEVICE : DMA_TO_DEVICE);

	return ret;
}

static int dwc2_submit_packet(struct dwc2 *dwc2, struct usb_device *dev, u8 hc,
			      unsigned long pipe, u8 *pid, int in, void *buf,
			      int len)
{
	int mps = usb_maxpacket(dev, pipe);
	int do_split = dwc2_do_split(dwc2, dev);
	int complete_split = 0;
	int done = 0;
	int ret = 0;
	uint32_t xfer_len;
	uint32_t num_packets;
	int stop_transfer = 0;
	uint32_t max_xfer_len;
	int ssplit_frame_num = 0;

	max_xfer_len = dwc2->params.max_packet_count * mps;

	if (max_xfer_len > dwc2->params.max_transfer_size)
		max_xfer_len = dwc2->params.max_transfer_size;

	/* Make sure that max_xfer_len is a multiple of max packet size. */
	num_packets = max_xfer_len / mps;
	max_xfer_len = num_packets * mps;

	/* Initialize channel */
	dwc2_hc_init(dwc2, dev, hc, pipe, in);

	/* Check if the target is a FS/LS device behind a HS hub */
	if (do_split) {
		dwc2_hc_init_split(dwc2, dev, hc);
		num_packets = 1;
		max_xfer_len = mps;
	}
	do {
		int actual_len = 0;
		uint32_t hcint, hcsplt;
		int odd_frame = 0;

		xfer_len = len - done;

		if (xfer_len > max_xfer_len)
			xfer_len = max_xfer_len;
		else if (xfer_len > mps)
			num_packets = (xfer_len + mps - 1) / mps;
		else
			num_packets = 1;

		if (complete_split || do_split) {
			hcsplt = dwc2_readl(dwc2, HCSPLT(hc));
			if (complete_split)
				hcsplt |= HCSPLT_COMPSPLT;
			else if (do_split)
				hcsplt &= ~HCSPLT_COMPSPLT;
			dwc2_writel(dwc2, hcsplt, HCSPLT(hc));
		}

		if (usb_pipeint(pipe)) {
			int uframe_num = dwc2_readl(dwc2, HFNUM);

			if (!(uframe_num & 0x1))
				odd_frame = 1;
		}

		ret = transfer_chunk(dwc2, hc, pid,
				     in, (char *)buf + done, num_packets,
				     xfer_len, &actual_len, odd_frame);

		hcint = dwc2_readl(dwc2, HCINT(hc));
		if (complete_split) {
			stop_transfer = 0;
			if (hcint & HCINTMSK_NYET) {
				int frame_num = HFNUM_MAX_FRNUM &
					dwc2_readl(dwc2, HFNUM);

				ret = 0;
				if (((frame_num - ssplit_frame_num) &
				    HFNUM_MAX_FRNUM) > 4)
					ret = -EAGAIN;
			} else {
				complete_split = 0;
			}
		} else if (do_split) {
			if (hcint & HCINTMSK_ACK) {
				ssplit_frame_num = HFNUM_MAX_FRNUM &
					dwc2_readl(dwc2, HFNUM);
				ret = 0;
				complete_split = 1;
			}
		}

		if (ret)
			break;

		if (actual_len < xfer_len)
			stop_transfer = 1;

		done += actual_len;

	/* Transactions are done when when either all data is transferred or
	 * there is a short transfer. In case of a SPLIT make sure the CSPLIT
	 * is executed.
	 */
	} while (((done < len) && !stop_transfer) || complete_split);

	dwc2_writel(dwc2, 0, HCINTMSK(hc));
	dwc2_writel(dwc2, 0xFFFFFFFF, HCINT(hc));

	dev->status = 0;
	dev->act_len = done;

	return ret;
}

static int dwc2_submit_control_msg(struct usb_device *udev,
				   unsigned long pipe, void *buffer, int len,
				   struct devrequest *setup, int timeout)
{
	struct usb_host *host = udev->host;
	struct dwc2 *dwc2 = to_dwc2(host);
	int devnum = usb_pipedevice(pipe);
	int ret, act_len;
	u8 pid;
	u8 hc = DWC2_HC_CHANNEL;
	/* For CONTROL endpoint pid should start with DATA1 */
	int status_direction;

	if (devnum == dwc2->root_hub_devnum) {
		udev->speed = USB_SPEED_HIGH;
		ret = dwc2_submit_roothub(dwc2, udev, pipe, buffer, len, setup);
		return ret;
	}

	/* SETUP stage */
	pid = TSIZ_SC_MC_PID_SETUP;
	do {
		ret = dwc2_submit_packet(dwc2, udev, hc, pipe, &pid,
					 0, setup, 8);
	} while (ret == -EAGAIN);
	if (ret)
		return ret;

	/* DATA stage */
	act_len = 0;
	if (buffer) {
		pid = TSIZ_SC_MC_PID_DATA1;
		do {
			ret = dwc2_submit_packet(dwc2, udev, hc, pipe, &pid,
						 usb_pipein(pipe), buffer, len);
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
	pid = TSIZ_SC_MC_PID_DATA1;
	do {
		ret = dwc2_submit_packet(dwc2, udev, hc, pipe, &pid,
					 status_direction, NULL, 0);
	} while (ret == -EAGAIN);
	if (ret)
		return ret;

	if (setup->requesttype == USB_RECIP_ENDPOINT
	    && setup->request == USB_REQ_CLEAR_FEATURE) {
		/* From USB 2.0, section 9.4.5:
		 * ClearFeature(ENDPOINT_HALT) request always results
		 * in the data toggle being reinitialized to DATA0.
		 */
		int ep = le16_to_cpu(setup->index) & 0xf;
		dwc2_endpoint_reset(dwc2, usb_pipein(pipe), devnum, ep);
	}

	udev->act_len = act_len;
	udev->status = 0;

	return 0;
}

static int dwc2_submit_bulk_msg(struct usb_device *udev, unsigned long pipe,
				void *buffer, int len, int timeout)
{
	struct usb_host *host = udev->host;
	struct dwc2 *dwc2 = to_dwc2(host);
	int devnum = usb_pipedevice(pipe);
	int ep = usb_pipeendpoint(pipe);
	int in = usb_pipein(pipe);
	u8 *pid;
	u8 hc = DWC2_HC_CHANNEL;
	uint64_t start;
	int ret;

	if ((devnum >= MAX_DEVICE) || (devnum == dwc2->root_hub_devnum)) {
		udev->status = 0;
		return -EINVAL;
	}

	if (in)
		pid = &dwc2->in_data_toggle[devnum][ep];
	else
		pid = &dwc2->out_data_toggle[devnum][ep];

	start = get_time_ns();
	do {
		ret = dwc2_submit_packet(dwc2, udev, hc, pipe, pid, in,
					 buffer, len);
	} while (ret == -EAGAIN && !is_timeout(start, timeout * MSECOND));
	if (ret == -EAGAIN) {
		dwc2_err(dwc2, "Timeout on bulk endpoint\n");
		ret = -ETIMEDOUT;
	}

	dwc2_dbg(dwc2, "%s: return %d\n", __func__, ret);

	return ret;
}

static int dwc2_submit_int_msg(struct usb_device *udev, unsigned long pipe,
			       void *buffer, int len, int interval)
{
	struct usb_host *host = udev->host;
	struct dwc2 *dwc2 = to_dwc2(host);
	int devnum = usb_pipedevice(pipe);
	int ep = usb_pipeendpoint(pipe);
	int in = usb_pipein(pipe);
	u8 *pid;
	u8 hc = DWC2_HC_CHANNEL;
	uint64_t start;
	int ret;

	if ((devnum >= MAX_DEVICE) || (devnum == dwc2->root_hub_devnum)) {
		udev->status = 0;
		return -EINVAL;
	}

	if (usb_pipein(pipe))
		pid = &dwc2->in_data_toggle[devnum][ep];
	else
		pid = &dwc2->out_data_toggle[devnum][ep];

	start = get_time_ns();

	while (1) {
		ret = dwc2_submit_packet(dwc2, udev, hc, pipe, pid, in,
					 buffer, len);
		if (ret != -EAGAIN)
			return ret;
		if (is_timeout(start, USB_CNTL_TIMEOUT * MSECOND)) {
			dwc2_err(dwc2, "Timeout on interrupt endpoint\n");
			return -ETIMEDOUT;
		}
	}
}

/**
 * dwc2_calculate_dynamic_fifo() - Calculates the default fifo size
 * For system that have a total fifo depth that is smaller than the default
 * RX + TX fifo size.
 *
 * @dwc2: Programming view of DWC_otg controller
 */
static void dwc2_calculate_dynamic_fifo(struct dwc2 *dwc2)
{
	struct dwc2_core_params *params = &dwc2->params;
	struct dwc2_hw_params *hw = &dwc2->hw_params;
	u32 rxfsiz, nptxfsiz, ptxfsiz, total_fifo_size;

	total_fifo_size = hw->total_fifo_size;
	rxfsiz = params->host_rx_fifo_size;
	nptxfsiz = params->host_nperio_tx_fifo_size;
	ptxfsiz = params->host_perio_tx_fifo_size;

	/*
	 * Will use Method 2 defined in the DWC2 spec: minimum FIFO depth
	 * allocation with support for high bandwidth endpoints. Synopsys
	 * defines MPS(Max Packet size) for a periodic EP=1024, and for
	 * non-periodic as 512.
	 */
	if (total_fifo_size < (rxfsiz + nptxfsiz + ptxfsiz)) {
		/*
		 * For Buffer DMA mode/Scatter Gather DMA mode
		 * 2 * ((Largest Packet size / 4) + 1 + 1) + n
		 * with n = number of host channel.
		 * 2 * ((1024/4) + 2) = 516
		 */
		rxfsiz = 516 + hw->host_channels;

		/*
		 * min non-periodic tx fifo depth
		 * 2 * (largest non-periodic USB packet used / 4)
		 * 2 * (512/4) = 256
		 */
		nptxfsiz = 256;

		/*
		 * min periodic tx fifo depth
		 * (largest packet size*MC)/4
		 * (1024 * 3)/4 = 768
		 */
		ptxfsiz = 768;
	}

	params->host_rx_fifo_size = rxfsiz;
	params->host_nperio_tx_fifo_size = nptxfsiz;
	params->host_perio_tx_fifo_size = ptxfsiz;

	/*
	 * If the summation of RX, NPTX and PTX fifo sizes is still
	 * bigger than the total_fifo_size, then we have a problem.
	 *
	 * We won't be able to allocate as many endpoints. Right now,
	 * we're just printing an error message, but ideally this FIFO
	 * allocation algorithm would be improved in the future.
	 *
	 * FIXME improve this FIFO allocation algorithm.
	 */
	if (unlikely(total_fifo_size < (rxfsiz + nptxfsiz + ptxfsiz)))
		dwc2_err(dwc2, "invalid fifo sizes\n");
}

static void dwc2_config_fifos(struct dwc2 *dwc2)
{
	struct dwc2_core_params *params = &dwc2->params;
	u32 nptxfsiz, hptxfsiz, dfifocfg, grxfsiz;

	if (!params->enable_dynamic_fifo)
		return;

	dwc2_calculate_dynamic_fifo(dwc2);

	/* Rx FIFO */
	grxfsiz = dwc2_readl(dwc2, GRXFSIZ);
	dwc2_dbg(dwc2, "initial grxfsiz=%08x\n", grxfsiz);
	grxfsiz &= ~GRXFSIZ_DEPTH_MASK;
	grxfsiz |= params->host_rx_fifo_size <<
		   GRXFSIZ_DEPTH_SHIFT & GRXFSIZ_DEPTH_MASK;
	dwc2_writel(dwc2, grxfsiz, GRXFSIZ);
	dwc2_dbg(dwc2, "new grxfsiz=%08x\n", dwc2_readl(dwc2, GRXFSIZ));

	/* Non-periodic Tx FIFO */
	dwc2_dbg(dwc2, "initial gnptxfsiz=%08x\n", dwc2_readl(dwc2, GNPTXFSIZ));
	nptxfsiz = params->host_nperio_tx_fifo_size <<
		   FIFOSIZE_DEPTH_SHIFT & FIFOSIZE_DEPTH_MASK;
	nptxfsiz |= params->host_rx_fifo_size <<
		    FIFOSIZE_STARTADDR_SHIFT & FIFOSIZE_STARTADDR_MASK;
	dwc2_writel(dwc2, nptxfsiz, GNPTXFSIZ);
	dwc2_dbg(dwc2, "new gnptxfsiz=%08x\n", dwc2_readl(dwc2, GNPTXFSIZ));

	/* Periodic Tx FIFO */
	dwc2_dbg(dwc2, "initial hptxfsiz=%08x\n", dwc2_readl(dwc2, HPTXFSIZ));
	hptxfsiz = params->host_perio_tx_fifo_size <<
		   FIFOSIZE_DEPTH_SHIFT & FIFOSIZE_DEPTH_MASK;
	hptxfsiz |= (params->host_rx_fifo_size +
		     params->host_nperio_tx_fifo_size) <<
		    FIFOSIZE_STARTADDR_SHIFT & FIFOSIZE_STARTADDR_MASK;
	dwc2_writel(dwc2, hptxfsiz, HPTXFSIZ);
	dwc2_dbg(dwc2, "new hptxfsiz=%08x\n", dwc2_readl(dwc2, HPTXFSIZ));

	if (dwc2->params.en_multiple_tx_fifo &&
	    dwc2->hw_params.snpsid >= DWC2_CORE_REV_2_91a) {
		/*
		 * This feature was implemented in 2.91a version
		 * Global DFIFOCFG calculation for Host mode -
		 * include RxFIFO, NPTXFIFO and HPTXFIFO
		 */
		dfifocfg = dwc2_readl(dwc2, GDFIFOCFG);
		dfifocfg &= ~GDFIFOCFG_EPINFOBASE_MASK;
		dfifocfg |= (params->host_rx_fifo_size +
			     params->host_nperio_tx_fifo_size +
			     params->host_perio_tx_fifo_size) <<
			    GDFIFOCFG_EPINFOBASE_SHIFT &
			    GDFIFOCFG_EPINFOBASE_MASK;
		dwc2_writel(dwc2, dfifocfg, GDFIFOCFG);
		dwc2_dbg(dwc2, "new dfifocfg=%08x\n", dfifocfg);
	}
}

/*
 * This function initializes the DWC2 controller registers for
 * host mode.
 *
 * This function flushes the Tx and Rx FIFOs and it flushes any entries in the
 * request queues. Host channels are reset to ensure that they are ready for
 * performing transfers.
 *
 * @param dev USB Device (NULL if driver model is not being used)
 * @param regs Programming view of DWC2 controller
 *
 */
static void dwc2_core_host_init(struct device_d *dev,
				   struct dwc2 *dwc2)
{
	uint32_t hcchar, hcfg, hprt0, hotgctl, usbcfg;
	int i, ret, num_channels;

	dwc2_dbg(dwc2, "%s(%p)\n", __func__, dwc2);

	/* Set HS/FS Timeout Calibration to 7 (max available value).
	 * The number of PHY clocks that the application programs in
	 * this field is added to the high/full speed interpacket timeout
	 * duration in the core to account for any additional delays
	 * introduced by the PHY. This can be required, because the delay
	 * introduced by the PHY in generating the linestate condition
	 * can vary from one PHY to another.
	 */
	usbcfg = dwc2_readl(dwc2, GUSBCFG);
	usbcfg |= GUSBCFG_TOUTCAL(7);
	dwc2_writel(dwc2, usbcfg, GUSBCFG);

	/* Restart the Phy Clock */
	dwc2_writel(dwc2, 0, PCGCTL);

	/* Initialize Host Configuration Register */
	dwc2_init_fs_ls_pclk_sel(dwc2);
	if (dwc2->params.speed == DWC2_SPEED_PARAM_FULL ||
	    dwc2->params.speed == DWC2_SPEED_PARAM_LOW) {
		hcfg = dwc2_readl(dwc2, HCFG);
		hcfg |= HCFG_FSLSSUPP;
		dwc2_writel(dwc2, hcfg, HCFG);
	}

	if (dwc2->params.dma_desc) {
		u32 op_mode = dwc2->hw_params.op_mode;

		if (dwc2->hw_params.snpsid < DWC2_CORE_REV_2_90a ||
		    !dwc2->hw_params.dma_desc_enable ||
		    op_mode == GHWCFG2_OP_MODE_SRP_CAPABLE_DEVICE ||
		    op_mode == GHWCFG2_OP_MODE_NO_SRP_CAPABLE_DEVICE ||
		    op_mode == GHWCFG2_OP_MODE_UNDEFINED) {
			dwc2_err(dwc2, "Descriptor DMA not suppported\n");
			dwc2_err(dwc2, "falling back to buffer DMA mode.\n");
			dwc2->params.dma_desc = false;
		} else {
			hcfg = dwc2_readl(dwc2, HCFG);
			hcfg |= HCFG_DESCDMA;
			dwc2_writel(dwc2, hcfg, HCFG);
		}
	}

	dwc2_config_fifos(dwc2);

	/* Clear Host Set HNP Enable in the OTG Control Register */
	hotgctl = dwc2_readl(dwc2, GOTGCTL);
	hotgctl &= ~GOTGCTL_HSTSETHNPEN;
	dwc2_writel(dwc2, hotgctl, GOTGCTL);

	/* Make sure the FIFOs are flushed. */
	dwc2_flush_all_fifo(dwc2);

	/* Flush out any leftover queued requests. */
	num_channels = dwc2->params.host_channels;
	for (i = 0; i < num_channels; i++) {
		hcchar = dwc2_readl(dwc2, HCCHAR(i));
		if (!(hcchar & HCCHAR_CHENA))
			continue;
		hcchar |= HCCHAR_CHDIS;
		hcchar &= ~(HCCHAR_CHENA | HCCHAR_EPDIR);
		dwc2_writel(dwc2, hcchar, HCCHAR(i));
	}

	/* Halt all channels to put them into a known state. */
	for (i = 0; i < num_channels; i++) {
		hcchar = dwc2_readl(dwc2, HCCHAR(i));
		if (!(hcchar & HCCHAR_CHENA))
			continue;
		hcchar |= HCCHAR_CHENA | HCCHAR_CHDIS;
		hcchar &= ~HCCHAR_EPDIR;

		dwc2_writel(dwc2, hcchar, HCCHAR(i));
		ret = dwc2_wait_bit_clear(dwc2, HCCHAR(i), HCCHAR_CHENA, 10000);
		if (ret)
			dwc2_warn(dwc2, "%s: Timeout! Reseting channel %d\n",
				  __func__, i);
	}

	/* Turn on the vbus power */
	if (dwc2_is_host_mode(dwc2)) {
		hprt0 = dwc2_readl(dwc2, HPRT0);
		hprt0 &= ~(HPRT0_ENA | HPRT0_CONNDET);
		hprt0 &= ~(HPRT0_ENACHG | HPRT0_OVRCURRCHG);
		if (!(hprt0 & HPRT0_PWR)) {
			hprt0 |= HPRT0_PWR;
			dwc2_writel(dwc2, hprt0, HPRT0);
		}
	}

	/* Disable all interrupts */
	dwc2_writel(dwc2, 0, GINTMSK);
	dwc2_writel(dwc2, 0, HAINTMSK);
}

static int dwc2_host_init(struct usb_host *host)
{
	struct dwc2 *dwc2 = to_dwc2(host);
	struct device_d *dev = dwc2->dev;
	uint32_t hprt0, gusbcfg;
	int i, j;

	/* Force Host mode in case the dwc2 controller is otg,
	 * otherwise the mode selection is dictated by the id
	 * pin, thus will require a otg A cable to be plugged-in.
	 */
	gusbcfg = dwc2_readl(dwc2, GUSBCFG) | GUSBCFG_FORCEHOSTMODE;
	dwc2_writel(dwc2, gusbcfg, GUSBCFG);
	mdelay(25);

	dwc2_core_init(dwc2);
	dwc2_core_host_init(dev, dwc2);

	hprt0 = dwc2_readl(dwc2, HPRT0);
	hprt0 &= ~(HPRT0_ENA | HPRT0_CONNDET);
	/* clear HPRT0_ENACHG and HPRT0_OVRCURRCHG by writing 1 */
	hprt0 |= HPRT0_ENACHG | HPRT0_OVRCURRCHG;
	hprt0 |= HPRT0_RST;
	dwc2_writel(dwc2, hprt0, HPRT0);

	mdelay(50);

	hprt0 = dwc2_readl(dwc2, HPRT0);
	hprt0 &= ~(HPRT0_ENA | HPRT0_CONNDET | HPRT0_RST);
	dwc2_writel(dwc2, hprt0, HPRT0);

	for (i = 0; i < MAX_DEVICE; i++) {
		for (j = 0; j < MAX_ENDPOINT; j++) {
			dwc2->in_data_toggle[i][j] = TSIZ_SC_MC_PID_DATA0;
			dwc2->out_data_toggle[i][j] = TSIZ_SC_MC_PID_DATA0;
		}
	}

	/*
	 * Add a 1 second delay here. This gives the host controller
	 * a bit time before the comminucation with the USB devices
	 * is started (the bus is scanned) and  fixes the USB detection
	 * problems with some problematic USB keys.
	 */
	if (dwc2_is_host_mode(dwc2))
		mdelay(1000);

	return 0;
}

static int dwc2_detect(struct device_d *dev)
{
	struct dwc2 *dwc2 = dev->priv;

	return usb_host_detect(&dwc2->host);
}

int dwc2_register_host(struct dwc2 *dwc2)
{
	struct usb_host *host;

	host = &dwc2->host;
	host->hw_dev = dwc2->dev;
	host->init = dwc2_host_init;
	host->submit_bulk_msg = dwc2_submit_bulk_msg;
	host->submit_control_msg = dwc2_submit_control_msg;
	host->submit_int_msg = dwc2_submit_int_msg;

	dwc2->dev->detect = dwc2_detect;

	return usb_register_host(host);
}

void dwc2_host_uninit(struct dwc2 *dwc2)
{
	uint32_t hprt0;

	hprt0 = dwc2_readl(dwc2, HPRT0);

	/* Put everything in reset. */
	hprt0 &= ~(HPRT0_ENA | HPRT0_ENACHG | HPRT0_CONNDET | HPRT0_OVRCURRCHG);
	hprt0 |= HPRT0_RST;

	dwc2_writel(dwc2, hprt0, HPRT0);
}
