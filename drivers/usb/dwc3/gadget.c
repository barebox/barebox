// SPDX-License-Identifier: GPL-2.0
/**
 * gadget.c - DesignWare USB3 DRD Controller Gadget Framework Link
 *
 * Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com
 *
 * Authors: Felipe Balbi <balbi@ti.com>,
 *	    Sebastian Andrzej Siewior <bigeasy@linutronix.de>
 *
 * Taken from Linux Kernel v3.19-rc1 (drivers/usb/dwc3/gadget.c) and ported
 * to uboot.
 *
 * commit 8e74475b0e : usb: dwc3: gadget: use udc-core's reset notifier
 */
#include <common.h>
#include <dma.h>
#include <io.h>
#include <linux/list.h>

#include <usb/gadget.h>
#include <usb/ch9.h>

#include "core.h"
#include "gadget.h"

#define DWC3_ALIGN_FRAME(d, n)	(((d)->frame_number + ((d)->interval * (n))) \
					& ~((d)->interval - 1))


/**
 * dwc3_gadget_set_test_mode - Enables USB2 Test Modes
 * @dwc: pointer to our context structure
 * @mode: the mode to set (J, K SE0 NAK, Force Enable)
 *
 * Caller should take care of locking. This function will
 * return 0 on success or -EINVAL if wrong Test Selector
 * is passed
 */
int dwc3_gadget_set_test_mode(struct dwc3 *dwc, int mode)
{
	u32		reg;

	reg = dwc3_readl(dwc->regs, DWC3_DCTL);
	reg &= ~DWC3_DCTL_TSTCTRL_MASK;

	switch (mode) {
	case TEST_J:
	case TEST_K:
	case TEST_SE0_NAK:
	case TEST_PACKET:
	case TEST_FORCE_EN:
		reg |= mode << 1;
		break;
	default:
		return -EINVAL;
	}

	dwc3_writel(dwc->regs, DWC3_DCTL, reg);

	return 0;
}

/**
 * dwc3_gadget_get_link_state - Gets current state of USB Link
 * @dwc: pointer to our context structure
 *
 * Caller should take care of locking. This function will
 * return the link state on success (>= 0) or -ETIMEDOUT.
 */
int dwc3_gadget_get_link_state(struct dwc3 *dwc)
{
	u32 reg;

	reg = dwc3_readl(dwc->regs, DWC3_DSTS);

	return DWC3_DSTS_USBLNKST(reg);
}

/**
 * dwc3_gadget_set_link_state - Sets USB Link to a particular State
 * @dwc: pointer to our context structure
 * @state: the state to put link into
 *
 * Caller should take care of locking. This function will
 * return 0 on success or -ETIMEDOUT.
 */
int dwc3_gadget_set_link_state(struct dwc3 *dwc, enum dwc3_link_state state)
{
	int retries = 10000;
	u32 reg;

	/*
	 * Wait until device controller is ready. Only applies to 1.94a and
	 * later RTL.
	 */
	if (dwc->revision >= DWC3_REVISION_194A) {
		while (--retries) {
			reg = dwc3_readl(dwc->regs, DWC3_DSTS);
			if (reg & DWC3_DSTS_DCNRD)
				udelay(5);
			else
				break;
		}

		if (retries <= 0)
			return -ETIMEDOUT;
	}

	reg = dwc3_readl(dwc->regs, DWC3_DCTL);
	reg &= ~DWC3_DCTL_ULSTCHNGREQ_MASK;

	/* set requested state */
	reg |= DWC3_DCTL_ULSTCHNGREQ(state);
	dwc3_writel(dwc->regs, DWC3_DCTL, reg);

	/*
	 * The following code is racy when called from dwc3_gadget_wakeup,
	 * and is not needed, at least on newer versions
	 */
	if (dwc->revision >= DWC3_REVISION_194A)
		return 0;

	/* wait for a change in DSTS */
	retries = 10000;
	while (--retries) {
		reg = dwc3_readl(dwc->regs, DWC3_DSTS);

		if (DWC3_DSTS_USBLNKST(reg) == state)
			return 0;

		udelay(5);
	}

	dev_dbg(dwc->dev, "link state change request timed out\n");

	return -ETIMEDOUT;
}

/**
 * dwc3_ep_inc_trb - increment a trb index.
 * @index: Pointer to the TRB index to increment.
 *
 * The index should never point to the link TRB. After incrementing,
 * if it is point to the link TRB, wrap around to the beginning. The
 * link TRB is always at the last TRB entry.
 */
static void dwc3_ep_inc_trb(u8 *index)
{
	(*index)++;
	if (*index == (DWC3_TRB_NUM - 1))
		*index = 0;
}

/**
 * dwc3_ep_inc_enq - increment endpoint's enqueue pointer
 * @dep: The endpoint whose enqueue pointer we're incrementing
 */
static void dwc3_ep_inc_enq(struct dwc3_ep *dep)
{
	dwc3_ep_inc_trb(&dep->trb_enqueue);
}

/**
 * dwc3_ep_inc_deq - increment endpoint's dequeue pointer
 * @dep: The endpoint whose enqueue pointer we're incrementing
 */
static void dwc3_ep_inc_deq(struct dwc3_ep *dep)
{
	dwc3_ep_inc_trb(&dep->trb_dequeue);
}

static void dwc3_gadget_del_and_unmap_request(struct dwc3_ep *dep,
					      struct dwc3_request *req,
					      int status)
{
	struct dwc3 *dwc = dep->dwc;

	list_del(&req->list);
	req->remaining = 0;
	req->needs_extra_trb = false;

	if (req->request.status == -EINPROGRESS)
		req->request.status = status;

	if (req->request.length == 0)
		return;

	if (req->trb)
		dma_unmap_single(dwc->dev, req->request.dma,
				 req->request.length,
			 req->direction ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

	req->trb = NULL;
}

/**
 * dwc3_gadget_giveback - call struct usb_request's ->complete callback
 * @dep: The endpoint to whom the request belongs to
 * @req: The request we're giving back
 * @status: completion code for the request
 *
 * Must be called with controller's lock held and interrupts disabled. This
 * function will unmap @req and call its ->complete() callback to notify upper
 * layers that it has completed.
 */
void dwc3_gadget_giveback(struct dwc3_ep *dep, struct dwc3_request *req,
			  int status)
{
	struct dwc3 *dwc = dep->dwc;

	dwc3_gadget_del_and_unmap_request(dep, req, status);
	dev_dbg(dwc->dev, "request %p from %s completed %d/%d ===> %d\n",
		req, dep->name, req->request.actual,
		req->request.length, status);
	req->status = DWC3_REQUEST_STATUS_COMPLETED;

	spin_unlock(&dwc->lock);
	req->request.complete(&dep->endpoint, &req->request);
	spin_lock(&dwc->lock);
}

/**
 * dwc3_send_gadget_generic_command - issue a generic command for the controller
 * @dwc: pointer to the controller context
 * @cmd: the command to be issued
 * @param: command parameter
 *
 * Caller should take care of locking. Issue @cmd with a given @param to @dwc
 * and wait for its completion.
 */
int dwc3_send_gadget_generic_command(struct dwc3 *dwc, unsigned cmd, u32 param)
{
	u32 timeout = 500;
	int status = 0;
	int ret = 0;
	u32 reg;

	dwc3_writel(dwc->regs, DWC3_DGCMDPAR, param);
	dwc3_writel(dwc->regs, DWC3_DGCMD, cmd | DWC3_DGCMD_CMDACT);

	do {
		reg = dwc3_readl(dwc->regs, DWC3_DGCMD);
		if (!(reg & DWC3_DGCMD_CMDACT)) {
			dev_dbg(dwc->dev, "%s: Command Complete --> %d\n",
				__func__,
				DWC3_DGCMD_STATUS(reg));
			status = DWC3_DGCMD_STATUS(reg);
			if (status)
				ret = -EINVAL;
			break;
		}

		udelay(1);
	} while (--timeout);

	if (!timeout) {
		ret = -ETIMEDOUT;
		status = -ETIMEDOUT;
	}

	return ret;
}

static int __dwc3_gadget_wakeup(struct dwc3 *dwc);
/**
 * dwc3_send_gadget_ep_cmd - issue an endpoint command
 * @dep: the endpoint to which the command is going to be issued
 * @cmd: the command to be issued
 * @params: parameters to the command
 *
 * Caller should handle locking. This function will issue @cmd with given
 * @params to @dep and wait for its completion.
 */
int dwc3_send_gadget_ep_cmd(struct dwc3_ep *dep, unsigned cmd,
			    struct dwc3_gadget_ep_cmd_params *params)
{
	const struct usb_endpoint_descriptor *desc = dep->endpoint.desc;
	struct dwc3 *dwc = dep->dwc;
	u32 timeout = 1000;
	u32 saved_config = 0;
	u32 reg;

	int cmd_status = 0;
	int ret = -EINVAL;

	/*
	 * When operating in USB 2.0 speeds (HS/FS), if GUSB2PHYCFG.ENBLSLPM or
	 * GUSB2PHYCFG.SUSPHY is set, it must be cleared before issuing an
	 * endpoint command.
	 *
	 * Save and clear both GUSB2PHYCFG.ENBLSLPM and GUSB2PHYCFG.SUSPHY
	 * settings. Restore them after the command is completed.
	 *
	 * DWC_usb3 3.30a and DWC_usb31 1.90a programming guide section 3.2.2
	 */
	if (dwc->gadget.speed <= USB_SPEED_HIGH) {
		reg = dwc3_readl(dwc->regs, DWC3_GUSB2PHYCFG(0));
		if (unlikely(reg & DWC3_GUSB2PHYCFG_SUSPHY)) {
			saved_config |= DWC3_GUSB2PHYCFG_SUSPHY;
			reg &= ~DWC3_GUSB2PHYCFG_SUSPHY;
		}

		if (reg & DWC3_GUSB2PHYCFG_ENBLSLPM) {
			saved_config |= DWC3_GUSB2PHYCFG_ENBLSLPM;
			reg &= ~DWC3_GUSB2PHYCFG_ENBLSLPM;
		}

		if (saved_config)
			dwc3_writel(dwc->regs, DWC3_GUSB2PHYCFG(0), reg);
	}

	if (DWC3_DEPCMD_CMD(cmd) == DWC3_DEPCMD_STARTTRANSFER) {
		int		needs_wakeup;

		needs_wakeup = (dwc->link_state == DWC3_LINK_STATE_U1 ||
				dwc->link_state == DWC3_LINK_STATE_U2 ||
				dwc->link_state == DWC3_LINK_STATE_U3);

		if (unlikely(needs_wakeup)) {
			ret = __dwc3_gadget_wakeup(dwc);
			dev_warn(dwc->dev, "wakeup failed --> %d\n", ret);
		}
	}

	dwc3_writel(dep->regs, DWC3_DEPCMDPAR0, params->param0);
	dwc3_writel(dep->regs, DWC3_DEPCMDPAR1, params->param1);
	dwc3_writel(dep->regs, DWC3_DEPCMDPAR2, params->param2);

	/*
	 * Synopsys Databook 2.60a states in section 6.3.2.5.6 of that if we're
	 * not relying on XferNotReady, we can make use of a special "No
	 * Response Update Transfer" command where we should clear both CmdAct
	 * and CmdIOC bits.
	 *
	 * With this, we don't need to wait for command completion and can
	 * straight away issue further commands to the endpoint.
	 *
	 * NOTICE: We're making an assumption that control endpoints will never
	 * make use of Update Transfer command. This is a safe assumption
	 * because we can never have more than one request at a time with
	 * Control Endpoints. If anybody changes that assumption, this chunk
	 * needs to be updated accordingly.
	 */
	if (DWC3_DEPCMD_CMD(cmd) == DWC3_DEPCMD_UPDATETRANSFER &&
			!usb_endpoint_xfer_isoc(desc))
		cmd &= ~(DWC3_DEPCMD_CMDIOC | DWC3_DEPCMD_CMDACT);
	else
		cmd |= DWC3_DEPCMD_CMDACT;

	dwc3_writel(dep->regs, DWC3_DEPCMD, cmd);
	do {
		reg = dwc3_readl(dep->regs, DWC3_DEPCMD);
		if (!(reg & DWC3_DEPCMD_CMDACT)) {
			cmd_status = DWC3_DEPCMD_STATUS(reg);

			switch (cmd_status) {
			case 0:
				ret = 0;
				break;
			case DEPEVT_TRANSFER_NO_RESOURCE:
				ret = -EINVAL;
				break;
			case DEPEVT_TRANSFER_BUS_EXPIRY:
				/*
				 * SW issues START TRANSFER command to
				 * isochronous ep with future frame interval. If
				 * future interval time has already passed when
				 * core receives the command, it will respond
				 * with an error status of 'Bus Expiry'.
				 *
				 * Instead of always returning -EINVAL, let's
				 * give a hint to the gadget driver that this is
				 * the case by returning -EAGAIN.
				 */
				ret = -EAGAIN;
				break;
			default:
				dev_warn(dwc->dev, "UNKNOWN cmd status\n");
			}

			break;
		}
	} while (--timeout);

	if (timeout == 0) {
		ret = -ETIMEDOUT;
		cmd_status = -ETIMEDOUT;
	}

	if (ret == 0 && DWC3_DEPCMD_CMD(cmd) == DWC3_DEPCMD_STARTTRANSFER) {
		dep->flags |= DWC3_EP_TRANSFER_STARTED;
		dwc3_gadget_ep_get_transfer_index(dep);
	}

	if (saved_config) {
		reg = dwc3_readl(dwc->regs, DWC3_GUSB2PHYCFG(0));
		reg |= saved_config;
		dwc3_writel(dwc->regs, DWC3_GUSB2PHYCFG(0), reg);
	}

	return ret;

}

static int dwc3_send_clear_stall_ep_cmd(struct dwc3_ep *dep)
{
	struct dwc3 *dwc = dep->dwc;
	struct dwc3_gadget_ep_cmd_params params;
	u32 cmd = DWC3_DEPCMD_CLEARSTALL;

	/*
	 * As of core revision 2.60a the recommended programming model
	 * is to set the ClearPendIN bit when issuing a Clear Stall EP
	 * command for IN endpoints. This is to prevent an issue where
	 * some (non-compliant) hosts may not send ACK TPs for pending
	 * IN transfers due to a mishandled error condition. Synopsys
	 * STAR 9000614252.
	 */
	if (dep->direction && (dwc->revision >= DWC3_REVISION_260A) &&
	    (dwc->gadget.speed >= USB_SPEED_SUPER))
		cmd |= DWC3_DEPCMD_CLEARPENDIN;

	memset(&params, 0, sizeof(params));

	return dwc3_send_gadget_ep_cmd(dep, cmd, &params);
}

static dma_addr_t dwc3_trb_dma_offset(struct dwc3_ep *dep,
				      struct dwc3_trb *trb)
{
	u32 offset = (char *) trb - (char *) dep->trb_pool;

	return dep->trb_pool_dma + offset;
}

static int dwc3_alloc_trb_pool(struct dwc3_ep *dep)
{
	if (dep->trb_pool)
		return 0;

	dep->trb_pool = dma_alloc_coherent(sizeof(struct dwc3_trb) *
					  DWC3_TRB_NUM,
					  &dep->trb_pool_dma);
	if (!dep->trb_pool) {
		dev_err(dep->dwc->dev, "failed to allocate trb pool for %s\n",
			dep->name);
		return -ENOMEM;
	}

	return 0;
}

static void dwc3_free_trb_pool(struct dwc3_ep *dep)
{
	dma_free_coherent(dep->trb_pool, 0, sizeof(dma_addr_t));

	dep->trb_pool = NULL;
	dep->trb_pool_dma = 0;
}

static int dwc3_gadget_set_xfer_resource(struct dwc3_ep *dep)
{
	struct dwc3_gadget_ep_cmd_params params;

	memset(&params, 0x00, sizeof(params));

	params.param0 = DWC3_DEPXFERCFG_NUM_XFER_RES(1);

	return dwc3_send_gadget_ep_cmd(dep, DWC3_DEPCMD_SETTRANSFRESOURCE,
				       &params);
}

/**
 * dwc3_gadget_start_config - configure ep resources
 * @dep: endpoint that is being enabled
 *
 * Issue a %DWC3_DEPCMD_DEPSTARTCFG command to @dep. After the command's
 * completion, it will set Transfer Resource for all available endpoints.
 *
 * The assignment of transfer resources cannot perfectly follow the data book
 * due to the fact that the controller driver does not have all knowledge of the
 * configuration in advance. It is given this information piecemeal by the
 * composite gadget framework after every SET_CONFIGURATION and
 * SET_INTERFACE. Trying to follow the databook programming model in this
 * scenario can cause errors. For two reasons:
 *
 * 1) The databook says to do %DWC3_DEPCMD_DEPSTARTCFG for every
 * %USB_REQ_SET_CONFIGURATION and %USB_REQ_SET_INTERFACE (8.1.5). This is
 * incorrect in the scenario of multiple interfaces.
 *
 * 2) The databook does not mention doing more %DWC3_DEPCMD_DEPXFERCFG for new
 * endpoint on alt setting (8.1.6).
 *
 * The following simplified method is used instead:
 *
 * All hardware endpoints can be assigned a transfer resource and this setting
 * will stay persistent until either a core reset or hibernation. So whenever we
 * do a %DWC3_DEPCMD_DEPSTARTCFG(0) we can go ahead and do
 * %DWC3_DEPCMD_DEPXFERCFG for every hardware endpoint as well. We are
 * guaranteed that there are as many transfer resources as endpoints.
 *
 * This function is called for each endpoint when it is being enabled but is
 * triggered only when called for EP0-out, which always happens first, and which
 * should only happen in one of the above conditions.
 */
static int dwc3_gadget_start_config(struct dwc3_ep *dep)
{
	struct dwc3_gadget_ep_cmd_params params;
	struct dwc3 *dwc;
	u32 cmd;
	int i;
	int ret;

	if (dep->number)
		return 0;

	memset(&params, 0x00, sizeof(params));
	cmd = DWC3_DEPCMD_DEPSTARTCFG;
	dwc = dep->dwc;

	ret = dwc3_send_gadget_ep_cmd(dep, cmd, &params);
	if (ret)
		return ret;

	for (i = 0; i < DWC3_ENDPOINTS_NUM; i++) {
		struct dwc3_ep *dep = dwc->eps[i];

		if (!dep)
			continue;

		ret = dwc3_gadget_set_xfer_resource(dep);
		if (ret)
			return ret;
	}

	return 0;
}

static int dwc3_gadget_set_ep_config(struct dwc3_ep *dep, unsigned int action)
{
	const struct usb_ss_ep_comp_descriptor *comp_desc;
	const struct usb_endpoint_descriptor *desc;
	struct dwc3_gadget_ep_cmd_params params;
	struct dwc3 *dwc = dep->dwc;

	comp_desc = dep->endpoint.comp_desc;
	desc = dep->endpoint.desc;

	memset(&params, 0x00, sizeof(params));

	params.param0 = DWC3_DEPCFG_EP_TYPE(usb_endpoint_type(desc))
		| DWC3_DEPCFG_MAX_PACKET_SIZE(usb_endpoint_maxp(desc));

	/* Burst size is only needed in SuperSpeed mode */
	if (dwc->gadget.speed == USB_SPEED_SUPER) {
		u32 burst = dep->endpoint.maxburst - 1;
		params.param0 |= DWC3_DEPCFG_BURST_SIZE(burst);
	}

	params.param0 |= action;
	if (action == DWC3_DEPCFG_ACTION_RESTORE)
		params.param2 |= dep->saved_state;

	if (usb_endpoint_xfer_control(desc))
		params.param1 = DWC3_DEPCFG_XFER_COMPLETE_EN;

	if (dep->number <= 1 || usb_endpoint_xfer_isoc(desc))
		params.param1 |= DWC3_DEPCFG_XFER_NOT_READY_EN;

	if (usb_ss_max_streams(comp_desc) && usb_endpoint_xfer_bulk(desc)) {
		params.param1 |= DWC3_DEPCFG_STREAM_CAPABLE
			| DWC3_DEPCFG_STREAM_EVENT_EN;
		dep->stream_capable = true;
	}

	if (!usb_endpoint_xfer_control(desc))
		params.param1 |= DWC3_DEPCFG_XFER_IN_PROGRESS_EN;

	/*
	 * We are doing 1:1 mapping for endpoints, meaning
	 * Physical Endpoints 2 maps to Logical Endpoint 2 and
	 * so on. We consider the direction bit as part of the physical
	 * endpoint number. So USB endpoint 0x81 is 0x03.
	 */
	params.param1 |= DWC3_DEPCFG_EP_NUMBER(dep->number);

	/*
	 * We must use the lower 16 TX FIFOs even though
	 * HW might have more
	 */
	if (dep->direction)
		params.param0 |= DWC3_DEPCFG_FIFO_NUMBER(dep->number >> 1);

	if (desc->bInterval) {
		params.param1 |= DWC3_DEPCFG_BINTERVAL_M1(desc->bInterval - 1);
		dep->interval = 1 << (desc->bInterval - 1);
	}

	return dwc3_send_gadget_ep_cmd(dep, DWC3_DEPCMD_SETEPCONFIG, &params);
}

/**
 * __dwc3_gadget_ep_enable - Initializes a HW endpoint
 * @dep: endpoint to be initialized
 * @desc: USB Endpoint Descriptor
 *
 * Caller should take care of locking
 */
static int __dwc3_gadget_ep_enable(struct dwc3_ep *dep, unsigned int action)
{
	const struct usb_endpoint_descriptor *desc = dep->endpoint.desc;
	struct dwc3 *dwc = dep->dwc;

	u32 reg;
	int ret;

	dev_dbg(dwc->dev, "Enabling %s\n", dep->name);

	if (!(dep->flags & DWC3_EP_ENABLED)) {
		ret = dwc3_gadget_start_config(dep);
		if (ret)
			return ret;
	}

	ret = dwc3_gadget_set_ep_config(dep, action);
	if (ret)
		return ret;

	if (!(dep->flags & DWC3_EP_ENABLED)) {
		struct dwc3_trb *trb_st_hw;
		struct dwc3_trb *trb_link;

		dep->type = usb_endpoint_type(desc);
		dep->flags |= DWC3_EP_ENABLED;

		reg = dwc3_readl(dwc->regs, DWC3_DALEPENA);
		reg |= DWC3_DALEPENA_EP(dep->number);
		dwc3_writel(dwc->regs, DWC3_DALEPENA, reg);

		if (usb_endpoint_xfer_control(desc))
			return 0;

		/* Initialize the TRB ring */
		dep->trb_dequeue = 0;
		dep->trb_enqueue = 0;
		memset(dep->trb_pool, 0,
		       sizeof(struct dwc3_trb) * DWC3_TRB_NUM);

		/* Link TRB. The HWO bit is never reset */
		trb_st_hw = &dep->trb_pool[0];

		trb_link = &dep->trb_pool[DWC3_TRB_NUM - 1];
		memset(trb_link, 0, sizeof(*trb_link));

		trb_link->bpl = lower_32_bits(dwc3_trb_dma_offset(dep,
								 trb_st_hw));
		trb_link->bph = upper_32_bits(dwc3_trb_dma_offset(dep,
								 trb_st_hw));
		trb_link->ctrl |= DWC3_TRBCTL_LINK_TRB;
		trb_link->ctrl |= DWC3_TRB_CTRL_HWO;
	}

	/*
	 * Issue StartTransfer here with no-op TRB so we can always rely on No
	 * Response Update Transfer command.
	 */
	if ((usb_endpoint_xfer_bulk(desc) && !dep->stream_capable) ||
			usb_endpoint_xfer_int(desc)) {
		struct dwc3_gadget_ep_cmd_params params;
		struct dwc3_trb *trb;
		dma_addr_t trb_dma;
		u32 cmd;

		memset(&params, 0, sizeof(params));
		trb = &dep->trb_pool[0];
		trb_dma = dwc3_trb_dma_offset(dep, trb);

		params.param0 = upper_32_bits(trb_dma);
		params.param1 = lower_32_bits(trb_dma);

		cmd = DWC3_DEPCMD_STARTTRANSFER;

		ret = dwc3_send_gadget_ep_cmd(dep, cmd, &params);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static void dwc3_stop_active_transfer(struct dwc3_ep *dep, bool force,
				      bool interrupt);
static void dwc3_remove_requests(struct dwc3 *dwc, struct dwc3_ep *dep)
{
	struct dwc3_request *req;

	dwc3_stop_active_transfer(dep, true, false);

	/* - giveback all requests to gadget driver */
	while (!list_empty(&dep->started_list)) {
		req = next_request(&dep->started_list);

		dwc3_gadget_giveback(dep, req, -ESHUTDOWN);
	}

	while (!list_empty(&dep->pending_list)) {
		req = next_request(&dep->pending_list);

		dwc3_gadget_giveback(dep, req, -ESHUTDOWN);
	}
}

/**
 * __dwc3_gadget_ep_disable - Disables a HW endpoint
 * @dep: the endpoint to disable
 *
 * This function also removes requests which are currently processed ny the
 * hardware and those which are not yet scheduled.
 * Caller should take care of locking.
 */
static int __dwc3_gadget_ep_disable(struct dwc3_ep *dep)
{
	struct dwc3 *dwc = dep->dwc;
	u32 reg;

	dwc3_remove_requests(dwc, dep);

	/* make sure HW endpoint isn't stalled */
	if (dep->flags & DWC3_EP_STALL)
		__dwc3_gadget_ep_set_halt(dep, 0, false);

	reg = dwc3_readl(dwc->regs, DWC3_DALEPENA);
	reg &= ~DWC3_DALEPENA_EP(dep->number);
	dwc3_writel(dwc->regs, DWC3_DALEPENA, reg);

	dep->stream_capable = false;
	dep->type = 0;
	dep->flags = 0;

	/* Clear out the ep descriptors for non-ep0 */
	if (dep->number > 1) {
		dep->endpoint.comp_desc = NULL;
		dep->endpoint.desc = NULL;
	}

	return 0;
}

/* -------------------------------------------------------------------------- */

static int dwc3_gadget_ep0_enable(struct usb_ep *ep,
				  const struct usb_endpoint_descriptor *desc)
{
	return -EINVAL;
}

static int dwc3_gadget_ep0_disable(struct usb_ep *ep)
{
	return -EINVAL;
}

/* -------------------------------------------------------------------------- */

static int dwc3_gadget_ep_enable(struct usb_ep *ep,
				 const struct usb_endpoint_descriptor *desc)
{
	struct dwc3_ep *dep;
	struct dwc3 *dwc;
	unsigned long flags;
	int ret;

	if (!ep || !desc || desc->bDescriptorType != USB_DT_ENDPOINT) {
		pr_debug("dwc3: invalid parameters\n");
		return -EINVAL;
	}

	if (!desc->wMaxPacketSize) {
		pr_debug("dwc3: missing wMaxPacketSize\n");
		return -EINVAL;
	}

	dep = to_dwc3_ep(ep);
	dwc = dep->dwc;

	if (dep->flags & DWC3_EP_ENABLED) {
		WARN(true, "%s is already enabled\n",
		     dep->name);
		return 0;
	}

	spin_lock_irqsave(&dwc->lock, flags);
	ret = __dwc3_gadget_ep_enable(dep, DWC3_DEPCFG_ACTION_INIT);
	spin_unlock_irqrestore(&dwc->lock, flags);

	return ret;
}

static int dwc3_gadget_ep_disable(struct usb_ep *ep)
{
	struct dwc3_ep *dep;
	struct dwc3 *dwc;
	unsigned long flags;
	int ret;

	if (!ep) {
		pr_debug("dwc3: invalid parameters\n");
		return -EINVAL;
	}

	dep = to_dwc3_ep(ep);
	dwc = dep->dwc;

	if (!(dep->flags & DWC3_EP_ENABLED)) {
		WARN(true, "%s is already disabled\n",
		     dep->name);
		return 0;
	}

	spin_lock_irqsave(&dwc->lock, flags);
	ret = __dwc3_gadget_ep_disable(dep);
	spin_unlock_irqrestore(&dwc->lock, flags);

	return ret;
}

static struct usb_request *dwc3_gadget_ep_alloc_request(struct usb_ep *ep)
{
	struct dwc3_request *req;
	struct dwc3_ep *dep = to_dwc3_ep(ep);

	req = xzalloc(sizeof(*req));
	if (!req)
		return NULL;

	req->direction	= dep->direction;
	req->epnum	= dep->number;
	req->dep	= dep;
	req->status	= DWC3_REQUEST_STATUS_UNKNOWN;

	return &req->request;
}

static void dwc3_gadget_ep_free_request(struct usb_ep *ep,
					struct usb_request *request)
{
	struct dwc3_request *req = to_dwc3_request(request);

	kfree(req);
}

/**
 * dwc3_ep_prev_trb - returns the previous TRB in the ring
 * @dep: The endpoint with the TRB ring
 * @index: The index of the current TRB in the ring
 *
 * Returns the TRB prior to the one pointed to by the index. If the
 * index is 0, we will wrap backwards, skip the link TRB, and return
 * the one just before that.
 */
static struct dwc3_trb *dwc3_ep_prev_trb(struct dwc3_ep *dep, u8 index)
{
	u8 tmp = index;

	if (!tmp)
		tmp = DWC3_TRB_NUM - 1;

	return &dep->trb_pool[tmp - 1];
}

static u32 dwc3_calc_trbs_left(struct dwc3_ep *dep)
{
	struct dwc3_trb *tmp;
	u8 trbs_left;

	/*
	 * If enqueue & dequeue are equal than it is either full or empty.
	 *
	 * One way to know for sure is if the TRB right before us has HWO bit
	 * set or not. If it has, then we're definitely full and can't fit any
	 * more transfers in our ring.
	 */
	if (dep->trb_enqueue == dep->trb_dequeue) {
		tmp = dwc3_ep_prev_trb(dep, dep->trb_enqueue);
		if (tmp->ctrl & DWC3_TRB_CTRL_HWO)
			return 0;

		return DWC3_TRB_NUM - 1;
	}

	trbs_left = dep->trb_dequeue - dep->trb_enqueue;
	trbs_left &= (DWC3_TRB_NUM - 1);

	if (dep->trb_dequeue < dep->trb_enqueue)
		trbs_left--;

	return trbs_left;
}

static void __dwc3_prepare_one_trb(struct dwc3_ep *dep, struct dwc3_trb *trb,
				   dma_addr_t dma, unsigned length,
				   unsigned chain, unsigned node,
				   unsigned stream_id, unsigned short_not_ok,
				   unsigned no_interrupt)
{
	struct dwc3 *dwc = dep->dwc;
	struct usb_gadget *gadget = &dwc->gadget;
	enum usb_device_speed speed = gadget->speed;

	trb->size = DWC3_TRB_SIZE_LENGTH(length);
	trb->bpl = lower_32_bits(dma);
	trb->bph = upper_32_bits(dma);

	switch (usb_endpoint_type(dep->endpoint.desc)) {
	case USB_ENDPOINT_XFER_CONTROL:
		trb->ctrl = DWC3_TRBCTL_CONTROL_SETUP;
		break;

	case USB_ENDPOINT_XFER_ISOC:
		if (!node) {
			trb->ctrl = DWC3_TRBCTL_ISOCHRONOUS_FIRST;

			/*
			 * USB Specification 2.0 Section 5.9.2 states that: "If
			 * there is only a single transaction in the microframe,
			 * only a DATA0 data packet PID is used.  If there are
			 * two transactions per microframe, DATA1 is used for
			 * the first transaction data packet and DATA0 is used
			 * for the second transaction data packet.  If there are
			 * three transactions per microframe, DATA2 is used for
			 * the first transaction data packet, DATA1 is used for
			 * the second, and DATA0 is used for the third."
			 *
			 * IOW, we should satisfy the following cases:
			 *
			 * 1) length <= maxpacket
			 *	- DATA0
			 *
			 * 2) maxpacket < length <= (2 * maxpacket)
			 *	- DATA1, DATA0
			 *
			 * 3) (2 * maxpacket) < length <= (3 * maxpacket)
			 *	- DATA2, DATA1, DATA0
			 */
			if (speed == USB_SPEED_HIGH) {
				struct usb_ep *ep = &dep->endpoint;
				unsigned int mult = 2;
				unsigned int maxp = usb_endpoint_maxp(ep->desc);

				if (length <= (2 * maxp))
					mult--;

				if (length <= maxp)
					mult--;

				trb->size |= DWC3_TRB_SIZE_PCM1(mult);
			}
		} else {
			trb->ctrl = DWC3_TRBCTL_ISOCHRONOUS;
		}

		/* always enable Interrupt on Missed ISOC */
		trb->ctrl |= DWC3_TRB_CTRL_ISP_IMI;
		break;

	case USB_ENDPOINT_XFER_BULK:
	case USB_ENDPOINT_XFER_INT:
		trb->ctrl = DWC3_TRBCTL_NORMAL;
		break;
	default:
		/*
		 * This is only possible with faulty memory because we
		 * checked it already :)
		 */
		dev_warn(dwc->dev, "Unknown endpoint type %d\n",
			 usb_endpoint_type(dep->endpoint.desc));
	}

	/*
	 * Enable Continue on Short Packet
	 * when endpoint is not a stream capable
	 */
	if (usb_endpoint_dir_out(dep->endpoint.desc)) {
		if (!dep->stream_capable)
			trb->ctrl |= DWC3_TRB_CTRL_CSP;

		if (short_not_ok)
			trb->ctrl |= DWC3_TRB_CTRL_ISP_IMI;
	}

	if ((!no_interrupt && !chain) ||
			(dwc3_calc_trbs_left(dep) == 1))
		trb->ctrl |= DWC3_TRB_CTRL_IOC;

	if (chain)
		trb->ctrl |= DWC3_TRB_CTRL_CHN;

	if (usb_endpoint_xfer_bulk(dep->endpoint.desc) && dep->stream_capable)
		trb->ctrl |= DWC3_TRB_CTRL_SID_SOFN(stream_id);

	trb->ctrl |= DWC3_TRB_CTRL_HWO;

	dwc3_ep_inc_enq(dep);
}

/**
 * dwc3_prepare_one_trb - setup one TRB from one request
 * @dep: endpoint for which this request is prepared
 * @req: dwc3_request pointer
 * @chain: should this TRB be chained to the next?
 * @node: only for isochronous endpoints. First TRB needs different type.
 */
static void dwc3_prepare_one_trb(struct dwc3_ep *dep,
				 struct dwc3_request *req,
				 unsigned chain, unsigned node)
{
	struct dwc3_trb *trb;
	unsigned int length;
	dma_addr_t dma;
	unsigned stream_id = req->request.stream_id;
	unsigned short_not_ok = req->request.short_not_ok;
	unsigned no_interrupt = req->request.no_interrupt;

	length = req->request.length;
	dma = req->request.dma;

	trb = &dep->trb_pool[dep->trb_enqueue];

	if (!req->trb) {
		dwc3_gadget_move_started_request(req);
		req->trb = trb;
		req->trb_dma = dwc3_trb_dma_offset(dep, trb);
	}

	req->num_trbs++;

	__dwc3_prepare_one_trb(dep, trb, dma, length, chain, node,
			stream_id, short_not_ok, no_interrupt);
}

static void dwc3_prepare_one_trb_linear(struct dwc3_ep *dep,
					struct dwc3_request *req)
{
	unsigned int length = req->request.length;
	unsigned int maxp = usb_endpoint_maxp(dep->endpoint.desc);
	unsigned int rem = length % maxp;

	if ((!length || rem) && usb_endpoint_dir_out(dep->endpoint.desc)) {
		struct dwc3 *dwc = dep->dwc;
		struct dwc3_trb *trb;

		req->needs_extra_trb = true;

		/* prepare normal TRB */
		dwc3_prepare_one_trb(dep, req, true, 0);

		/* Now prepare one extra TRB to align transfer size */
		trb = &dep->trb_pool[dep->trb_enqueue];
		req->num_trbs++;
		__dwc3_prepare_one_trb(dep, trb, dwc->bounce_addr, maxp - rem,
				false, 1, req->request.stream_id,
				req->request.short_not_ok,
				req->request.no_interrupt);
	} else if (req->request.zero && req->request.length &&
		   (IS_ALIGNED(req->request.length, maxp))) {
		struct dwc3 *dwc = dep->dwc;
		struct dwc3_trb *trb;

		req->needs_extra_trb = true;

		/* prepare normal TRB */
		dwc3_prepare_one_trb(dep, req, true, 0);

		/* Now prepare one extra TRB to handle ZLP */
		trb = &dep->trb_pool[dep->trb_enqueue];
		req->num_trbs++;
		__dwc3_prepare_one_trb(dep, trb, dwc->bounce_addr, 0,
				false, 1, req->request.stream_id,
				req->request.short_not_ok,
				req->request.no_interrupt);
	} else {
		dwc3_prepare_one_trb(dep, req, false, 0);
	}
}

/*
 * dwc3_prepare_trbs - setup TRBs from requests
 * @dep: endpoint for which requests are being prepared
 * @starting: true if the endpoint is idle and no requests are queued.
 *
 * The function goes through the requests list and sets up TRBs for the
 * transfers. The function returns once there are no more TRBs available or
 * it runs out of requests.
 */
static void dwc3_prepare_trbs(struct dwc3_ep *dep)
{
	struct dwc3_request *req, *n;
	struct dwc3 *dwc = dep->dwc;
	dma_addr_t dma_addr;

	BUILD_BUG_ON_NOT_POWER_OF_2(DWC3_TRB_NUM);

	list_for_each_entry_safe(req, n, &dep->pending_list, list) {
		dma_addr = dma_map_single(dwc->dev, req->request.buf,
					  req->request.length,
					  dep->number ?
					  DMA_TO_DEVICE : DMA_FROM_DEVICE);
		if (dma_mapping_error(dwc->dev, dma_addr))
			return;

		req->request.dma = dma_addr;

		dwc3_prepare_one_trb_linear(dep, req);

		if (!dwc3_calc_trbs_left(dep))
			return;
	}
}

static int __dwc3_gadget_kick_transfer(struct dwc3_ep *dep)
{
	struct dwc3_gadget_ep_cmd_params params;
	struct dwc3_request *req;
	int starting;
	int ret;
	u32 cmd;

	if (!dwc3_calc_trbs_left(dep))
		return 0;

	starting = !(dep->flags & DWC3_EP_TRANSFER_STARTED);

	dwc3_prepare_trbs(dep);

	req = next_request(&dep->started_list);
	if (!req) {
		dep->flags |= DWC3_EP_PENDING_REQUEST;
		return 0;
	}

	memset(&params, 0, sizeof(params));

	if (starting) {
		params.param0 = upper_32_bits(req->trb_dma);
		params.param1 = lower_32_bits(req->trb_dma);
		cmd = DWC3_DEPCMD_STARTTRANSFER;

		if (dep->stream_capable)
			cmd |= DWC3_DEPCMD_PARAM(req->request.stream_id);

		if (usb_endpoint_xfer_isoc(dep->endpoint.desc))
			cmd |= DWC3_DEPCMD_PARAM(dep->frame_number);
	} else {
		cmd = DWC3_DEPCMD_UPDATETRANSFER |
			DWC3_DEPCMD_PARAM(dep->resource_index);
	}

	ret = dwc3_send_gadget_ep_cmd(dep, cmd, &params);
	if (ret < 0) {
		/*
		 * FIXME we need to iterate over the list of requests
		 * here and stop, unmap, free and del each of the linked
		 * requests instead of what we do now.
		 */
		if (req->trb)
			memset(req->trb, 0, sizeof(struct dwc3_trb));
		dwc3_gadget_del_and_unmap_request(dep, req, ret);
		return ret;
	}

	return 0;
}

static int __dwc3_gadget_get_frame(struct dwc3 *dwc)
{
	u32 reg;

	reg = dwc3_readl(dwc->regs, DWC3_DSTS);
	return DWC3_DSTS_SOFFN(reg);
}

/**
 * dwc3_gadget_start_isoc_quirk - workaround invalid frame number
 * @dep: isoc endpoint
 *
 * This function tests for the correct combination of BIT[15:14] from the 16-bit
 * microframe number reported by the XferNotReady event for the future frame
 * number to start the isoc transfer.
 *
 * In DWC_usb31 version 1.70a-ea06 and prior, for highspeed and fullspeed
 * isochronous IN, BIT[15:14] of the 16-bit microframe number reported by the
 * XferNotReady event are invalid. The driver uses this number to schedule the
 * isochronous transfer and passes it to the START TRANSFER command. Because
 * this number is invalid, the command may fail. If BIT[15:14] matches the
 * internal 16-bit microframe, the START TRANSFER command will pass and the
 * transfer will start at the scheduled time, if it is off by 1, the command
 * will still pass, but the transfer will start 2 seconds in the future. For all
 * other conditions, the START TRANSFER command will fail with bus-expiry.
 *
 * In order to workaround this issue, we can test for the correct combination of
 * BIT[15:14] by sending START TRANSFER commands with different values of
 * BIT[15:14]: 'b00, 'b01, 'b10, and 'b11. Each combination is 2^14 uframe apart
 * (or 2 seconds). 4 seconds into the future will result in a bus-expiry status.
 * As the result, within the 4 possible combinations for BIT[15:14], there will
 * be 2 successful and 2 failure START COMMAND status. One of the 2 successful
 * command status will result in a 2-second delay start. The smaller BIT[15:14]
 * value is the correct combination.
 *
 * Since there are only 4 outcomes and the results are ordered, we can simply
 * test 2 START TRANSFER commands with BIT[15:14] combinations 'b00 and 'b01 to
 * deduce the smaller successful combination.
 *
 * Let test0 = test status for combination 'b00 and test1 = test status for 'b01
 * of BIT[15:14]. The correct combination is as follow:
 *
 * if test0 fails and test1 passes, BIT[15:14] is 'b01
 * if test0 fails and test1 fails, BIT[15:14] is 'b10
 * if test0 passes and test1 fails, BIT[15:14] is 'b11
 * if test0 passes and test1 passes, BIT[15:14] is 'b00
 *
 * Synopsys STAR 9001202023: Wrong microframe number for isochronous IN
 * endpoints.
 */
static int dwc3_gadget_start_isoc_quirk(struct dwc3_ep *dep)
{
	int cmd_status = 0;
	bool test0;
	bool test1;

	while (dep->combo_num < 2) {
		struct dwc3_gadget_ep_cmd_params params;
		u32 test_frame_number;
		u32 cmd;

		/*
		 * Check if we can start isoc transfer on the next interval or
		 * 4 uframes in the future with BIT[15:14] as dep->combo_num
		 */
		test_frame_number = dep->frame_number & 0x3fff;
		test_frame_number |= dep->combo_num << 14;
		test_frame_number += max_t(u32, 4, dep->interval);

		params.param0 = upper_32_bits(dep->dwc->bounce_addr);
		params.param1 = lower_32_bits(dep->dwc->bounce_addr);

		cmd = DWC3_DEPCMD_STARTTRANSFER;
		cmd |= DWC3_DEPCMD_PARAM(test_frame_number);
		cmd_status = dwc3_send_gadget_ep_cmd(dep, cmd, &params);

		/* Redo if some other failure beside bus-expiry is received */
		if (cmd_status && cmd_status != -EAGAIN) {
			dep->start_cmd_status = 0;
			dep->combo_num = 0;
			return 0;
		}

		/* Store the first test status */
		if (dep->combo_num == 0)
			dep->start_cmd_status = cmd_status;

		dep->combo_num++;

		/*
		 * End the transfer if the START_TRANSFER command is successful
		 * to wait for the next XferNotReady to test the command again
		 */
		if (cmd_status == 0) {
			dwc3_stop_active_transfer(dep, true, true);
			return 0;
		}
	}

	/* test0 and test1 are both completed at this point */
	test0 = (dep->start_cmd_status == 0);
	test1 = (cmd_status == 0);

	if (!test0 && test1)
		dep->combo_num = 1;
	else if (!test0 && !test1)
		dep->combo_num = 2;
	else if (test0 && !test1)
		dep->combo_num = 3;
	else if (test0 && test1)
		dep->combo_num = 0;

	dep->frame_number &= 0x3fff;
	dep->frame_number |= dep->combo_num << 14;
	dep->frame_number += max_t(u32, 4, dep->interval);

	/* Reinitialize test variables */
	dep->start_cmd_status = 0;
	dep->combo_num = 0;

	return __dwc3_gadget_kick_transfer(dep);
}

static int __dwc3_gadget_start_isoc(struct dwc3_ep *dep)
{
	struct dwc3 *dwc = dep->dwc;
	int ret;
	int i;

	if (list_empty(&dep->pending_list)) {
		dep->flags |= DWC3_EP_PENDING_REQUEST;
		return -EAGAIN;
	}

	if (!dwc->dis_start_transfer_quirk && dwc3_is_usb31(dwc) &&
	    (dwc->revision <= DWC3_USB31_REVISION_160A ||
	     (dwc->revision == DWC3_USB31_REVISION_170A &&
	      dwc->version_type >= DWC31_VERSIONTYPE_EA01 &&
	      dwc->version_type <= DWC31_VERSIONTYPE_EA06))) {

		if (dwc->gadget.speed <= USB_SPEED_HIGH && dep->direction)
			return dwc3_gadget_start_isoc_quirk(dep);
	}

	for (i = 0; i < DWC3_ISOC_MAX_RETRIES; i++) {
		dep->frame_number = DWC3_ALIGN_FRAME(dep, i + 1);

		ret = __dwc3_gadget_kick_transfer(dep);
		if (ret != -EAGAIN)
			break;
	}

	return ret;
}

static int __dwc3_gadget_ep_queue(struct dwc3_ep *dep, struct dwc3_request *req)
{
	struct dwc3 *dwc = dep->dwc;

	if (!dep->endpoint.desc) {
		dev_err(dwc->dev, "%s: can't queue to disabled endpoint\n",
				dep->name);
		return -ESHUTDOWN;
	}

	if (req->dep != dep) {
		WARN(true, "request %p belongs to '%s'\n",
				&req->request, req->dep->name);
		return -EINVAL;
	}

	if (req->status < DWC3_REQUEST_STATUS_COMPLETED) {
		WARN(true, "request %p already in flight\n", &req->request);
		return -EINVAL;
	}

	req->request.actual = 0;
	req->request.status = -EINPROGRESS;

	list_add_tail(&req->list, &dep->pending_list);
	req->status = DWC3_REQUEST_STATUS_QUEUED;

	/*
	 * NOTICE: Isochronous endpoints should NEVER be prestarted. We must
	 * wait for a XferNotReady event so we will know what's the current
	 * (micro-)frame number.
	 *
	 * Without this trick, we are very, very likely gonna get Bus Expiry
	 * errors which will force us issue EndTransfer command.
	 */
	if (usb_endpoint_xfer_isoc(dep->endpoint.desc)) {
		if (!(dep->flags & DWC3_EP_PENDING_REQUEST) &&
				!(dep->flags & DWC3_EP_TRANSFER_STARTED))
			return 0;

		if ((dep->flags & DWC3_EP_PENDING_REQUEST)) {
			if (!(dep->flags & DWC3_EP_TRANSFER_STARTED)) {
				return __dwc3_gadget_start_isoc(dep);
			}
		}
	}

	return __dwc3_gadget_kick_transfer(dep);
}

static int dwc3_gadget_ep_queue(struct usb_ep *ep, struct usb_request *request)
{
	struct dwc3_request *req = to_dwc3_request(request);
	struct dwc3_ep *dep = to_dwc3_ep(ep);

	unsigned long flags;

	int ret;

	spin_lock_irqsave(&dwc->lock, flags);
	ret = __dwc3_gadget_ep_queue(dep, req);
	spin_unlock_irqrestore(&dwc->lock, flags);

	return ret;
}

static void dwc3_gadget_ep_skip_trbs(struct dwc3_ep *dep,
				     struct dwc3_request *req)
{
	int i;

	/*
	 * If request was already started, this means we had to
	 * stop the transfer. With that we also need to ignore
	 * all TRBs used by the request, however TRBs can only
	 * be modified after completion of END_TRANSFER
	 * command. So what we do here is that we wait for
	 * END_TRANSFER completion and only after that, we jump
	 * over TRBs by clearing HWO and incrementing dequeue
	 * pointer.
	 */
	for (i = 0; i < req->num_trbs; i++) {
		struct dwc3_trb *trb;

		trb = req->trb + i;
		trb->ctrl &= ~DWC3_TRB_CTRL_HWO;
		dwc3_ep_inc_deq(dep);
	}

	req->num_trbs = 0;
}

static void dwc3_gadget_ep_cleanup_cancelled_requests(struct dwc3_ep *dep)
{
	struct dwc3_request *req;
	struct dwc3_request *tmp;

	list_for_each_entry_safe(req, tmp, &dep->cancelled_list, list) {
		dwc3_gadget_ep_skip_trbs(dep, req);
		dwc3_gadget_giveback(dep, req, -ECONNRESET);
	}
}

static int dwc3_gadget_ep_dequeue(struct usb_ep *ep,
				  struct usb_request *request)
{
	struct dwc3_request *req = to_dwc3_request(request);
	struct dwc3_request *r = NULL;
	struct dwc3_ep *dep = to_dwc3_ep(ep);
	struct dwc3 *dwc = dep->dwc;
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&dwc->lock, flags);

	list_for_each_entry(r, &dep->pending_list, list) {
		if (r == req)
			break;
	}

	if (r != req) {
		list_for_each_entry(r, &dep->started_list, list) {
			if (r == req)
				break;
		}
		if (r == req) {
			/* wait until it is processed */
			dwc3_stop_active_transfer(dep, true, true);

			if (!r->trb)
				goto out0;

			dwc3_gadget_move_cancelled_request(req);
			if (dep->flags & DWC3_EP_TRANSFER_STARTED)
				goto out0;
			else
				goto out1;
		}
		dev_err(dwc->dev, "request %p was not queued to %s\n",
				request, ep->name);
		ret = -EINVAL;
		goto out0;
	}

out1:
	/* giveback the request */
	dwc3_gadget_giveback(dep, req, -ECONNRESET);

out0:
	spin_unlock_irqrestore(&dwc->lock, flags);

	return ret;
}

int __dwc3_gadget_ep_set_halt(struct dwc3_ep *dep, int value, int protocol)
{
	struct dwc3_gadget_ep_cmd_params params;
	struct dwc3 *dwc = dep->dwc;
	int ret;

	if (usb_endpoint_xfer_isoc(dep->endpoint.desc)) {
		dev_err(dwc->dev, "%s is of Isochronous type\n", dep->name);
		return -EINVAL;
	}

	memset(&params, 0x00, sizeof(params));

	if (value) {
		struct dwc3_trb *trb;
		unsigned transfer_in_flight;
		unsigned started;

		if (dep->number > 1)
			trb = dwc3_ep_prev_trb(dep, dep->trb_enqueue);
		else
			trb = &dwc->ep0_trb[dep->trb_enqueue];

		transfer_in_flight = trb->ctrl & DWC3_TRB_CTRL_HWO;
		started = !list_empty(&dep->started_list);

		if (!protocol && ((dep->direction && transfer_in_flight) ||
				(!dep->direction && started))) {
			return -EAGAIN;
		}

		ret = dwc3_send_gadget_ep_cmd(dep, DWC3_DEPCMD_SETSTALL,
					      &params);
		if (ret)
			dev_err(dwc->dev, "failed to set STALL on %s\n",
					dep->name);
		else
			dep->flags |= DWC3_EP_STALL;
	} else {
		ret = dwc3_send_clear_stall_ep_cmd(dep);
		if (ret)
			dev_err(dwc->dev, "failed to clear STALL on %s\n",
					dep->name);
		else
			dep->flags &= ~(DWC3_EP_STALL | DWC3_EP_WEDGE);
	}

	return ret;
}

static int dwc3_gadget_ep_set_halt(struct usb_ep *ep, int value)
{
	struct dwc3_ep *dep = to_dwc3_ep(ep);
	unsigned long flags;

	int ret;

	spin_lock_irqsave(&dwc->lock, flags);
	ret = __dwc3_gadget_ep_set_halt(dep, value, false);
	spin_unlock_irqrestore(&dwc->lock, flags);

	return ret;
}

static int dwc3_gadget_ep_set_wedge(struct usb_ep *ep)
{
	struct dwc3_ep *dep = to_dwc3_ep(ep);
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&dwc->lock, flags);
	dep->flags |= DWC3_EP_WEDGE;

	if (dep->number == 0 || dep->number == 1)
		ret = __dwc3_gadget_ep0_set_halt(ep, 1);
	else
		ret = __dwc3_gadget_ep_set_halt(dep, 1, false);
	spin_unlock_irqrestore(&dwc->lock, flags);

	return ret;
}

/* -------------------------------------------------------------------------- */

static struct usb_endpoint_descriptor dwc3_gadget_ep0_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bmAttributes		= USB_ENDPOINT_XFER_CONTROL,
};

static const struct usb_ep_ops dwc3_gadget_ep0_ops = {
	.enable	= dwc3_gadget_ep0_enable,
	.disable	= dwc3_gadget_ep0_disable,
	.alloc_request	= dwc3_gadget_ep_alloc_request,
	.free_request	= dwc3_gadget_ep_free_request,
	.queue		= dwc3_gadget_ep0_queue,
	.dequeue	= dwc3_gadget_ep_dequeue,
	.set_halt	= dwc3_gadget_ep0_set_halt,
	.set_wedge	= dwc3_gadget_ep_set_wedge,
};

static const struct usb_ep_ops dwc3_gadget_ep_ops = {
	.enable	= dwc3_gadget_ep_enable,
	.disable	= dwc3_gadget_ep_disable,
	.alloc_request	= dwc3_gadget_ep_alloc_request,
	.free_request	= dwc3_gadget_ep_free_request,
	.queue		= dwc3_gadget_ep_queue,
	.dequeue	= dwc3_gadget_ep_dequeue,
	.set_halt	= dwc3_gadget_ep_set_halt,
	.set_wedge	= dwc3_gadget_ep_set_wedge,
};

/* -------------------------------------------------------------------------- */

static int dwc3_gadget_get_frame(struct usb_gadget *g)
{
	struct dwc3 *dwc = gadget_to_dwc(g);

	return __dwc3_gadget_get_frame(dwc);
}

static int __dwc3_gadget_wakeup(struct dwc3 *dwc)
{
	int retries;

	int ret;
	u32 reg;

	u8 link_state;
	u8 speed;

	/*
	 * According to the Databook Remote wakeup request should
	 * be issued only when the device is in early suspend state.
	 *
	 * We can check that via USB Link State bits in DSTS register.
	 */
	reg = dwc3_readl(dwc->regs, DWC3_DSTS);

	speed = reg & DWC3_DSTS_CONNECTSPD;
	if ((speed == DWC3_DSTS_SUPERSPEED) ||
	    (speed == DWC3_DSTS_SUPERSPEED_PLUS))
		return 0;

	link_state = DWC3_DSTS_USBLNKST(reg);

	switch (link_state) {
	case DWC3_LINK_STATE_RX_DET:	/* in HS, means Early Suspend */
	case DWC3_LINK_STATE_U3:	/* in HS, means SUSPEND */
		break;
	default:
		return -EINVAL;
	}

	ret = dwc3_gadget_set_link_state(dwc, DWC3_LINK_STATE_RECOV);
	if (ret < 0) {
		dev_err(dwc->dev, "failed to put link in Recovery\n");
		return ret;
	}

	/* Recent versions do this automatically */
	if (dwc->revision < DWC3_REVISION_194A) {
		/* write zeroes to Link Change Request */
		reg = dwc3_readl(dwc->regs, DWC3_DCTL);
		reg &= ~DWC3_DCTL_ULSTCHNGREQ_MASK;
		dwc3_writel(dwc->regs, DWC3_DCTL, reg);
	}

	/* poll until Link State changes to ON */
	retries = 20000;

	while (retries--) {
		reg = dwc3_readl(dwc->regs, DWC3_DSTS);

		/* in HS, means ON */
		if (DWC3_DSTS_USBLNKST(reg) == DWC3_LINK_STATE_U0)
			break;
	}

	if (DWC3_DSTS_USBLNKST(reg) != DWC3_LINK_STATE_U0) {
		dev_err(dwc->dev, "failed to send remote wakeup\n");
		return -EINVAL;
	}

	return 0;
}

static int dwc3_gadget_wakeup(struct usb_gadget *g)
{
	struct dwc3 *dwc = gadget_to_dwc(g);
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&dwc->lock, flags);
	ret = __dwc3_gadget_wakeup(dwc);
	spin_unlock_irqrestore(&dwc->lock, flags);

	return ret;
}

static int dwc3_gadget_set_selfpowered(struct usb_gadget *g,
				       int is_selfpowered)
{
	struct dwc3 *dwc = gadget_to_dwc(g);
	unsigned long flags;

	spin_lock_irqsave(&dwc->lock, flags);
	dwc->is_selfpowered = !!is_selfpowered;
	spin_unlock_irqrestore(&dwc->lock, flags);

	return 0;
}

static int dwc3_gadget_run_stop(struct dwc3 *dwc, int is_on, int suspend)
{
	u32 reg;
	u32 timeout = 500;

	reg = dwc3_readl(dwc->regs, DWC3_DCTL);
	if (is_on) {
		if (dwc->revision <= DWC3_REVISION_187A) {
			reg &= ~DWC3_DCTL_TRGTULST_MASK;
			reg |= DWC3_DCTL_TRGTULST_RX_DET;
		}

		if (dwc->revision >= DWC3_REVISION_194A)
			reg &= ~DWC3_DCTL_KEEP_CONNECT;
		reg |= DWC3_DCTL_RUN_STOP;

		if (dwc->has_hibernation)
			reg |= DWC3_DCTL_KEEP_CONNECT;

		dwc->pullups_connected = true;
	} else {
		reg &= ~DWC3_DCTL_RUN_STOP;

		if (dwc->has_hibernation && !suspend)
			reg &= ~DWC3_DCTL_KEEP_CONNECT;

		dwc->pullups_connected = false;
	}

	dwc3_writel(dwc->regs, DWC3_DCTL, reg);

	do {
		reg = dwc3_readl(dwc->regs, DWC3_DSTS);
		reg &= DWC3_DSTS_DEVCTRLHLT;
	} while (--timeout && !(!is_on ^ !reg));

	if (!timeout)
		return -ETIMEDOUT;

	dev_dbg(dwc->dev, "gadget %s data soft-%s\n",
			dwc->gadget_driver
			? dwc->gadget_driver->function : "no-function",
			is_on ? "connect" : "disconnect");

	return 0;
}

static int dwc3_gadget_pullup(struct usb_gadget *g, int is_on)
{
	struct dwc3 *dwc = gadget_to_dwc(g);
	unsigned long flags;
	int ret;

	is_on = !!is_on;

	/*
	 * Per databook, when we want to stop the gadget, if a control transfer
	 * is still in process, complete it and get the core into setup phase.
	 */
	if (!is_on && dwc->ep0state != EP0_SETUP_PHASE)
		dev_warn(dwc->dev, "not in SETUP phase\n");

	spin_lock_irqsave(&dwc->lock, flags);
	ret = dwc3_gadget_run_stop(dwc, is_on, false);
	spin_unlock_irqrestore(&dwc->lock, flags);

	return ret;
}

static void dwc3_gadget_enable_irq(struct dwc3 *dwc)
{
	u32 reg;

	/* Enable all but Start and End of Frame IRQs */
	reg = (DWC3_DEVTEN_VNDRDEVTSTRCVEDEN |
		DWC3_DEVTEN_EVNTOVERFLOWEN |
		DWC3_DEVTEN_CMDCMPLTEN |
		DWC3_DEVTEN_ERRTICERREN |
		DWC3_DEVTEN_WKUPEVTEN |
		DWC3_DEVTEN_ULSTCNGEN |
		DWC3_DEVTEN_CONNECTDONEEN |
		DWC3_DEVTEN_USBRSTEN |
		DWC3_DEVTEN_DISCONNEVTEN);

	if (dwc->revision < DWC3_REVISION_250A)
		reg |= DWC3_DEVTEN_ULSTCNGEN;

	dwc3_writel(dwc->regs, DWC3_DEVTEN, reg);
}

static void dwc3_gadget_disable_irq(struct dwc3 *dwc)
{
	/* mask all interrupts */
	dwc3_writel(dwc->regs, DWC3_DEVTEN, 0x00);
}

/**
 * dwc3_gadget_setup_nump - calculate and initialize NUMP field of %DWC3_DCFG
 * @dwc: pointer to our context structure
 *
 * The following looks like complex but it's actually very simple. In order to
 * calculate the number of packets we can burst at once on OUT transfers, we're
 * gonna use RxFIFO size.
 *
 * To calculate RxFIFO size we need two numbers:
 * MDWIDTH = size, in bits, of the internal memory bus
 * RAM2_DEPTH = depth, in MDWIDTH, of internal RAM2 (where RxFIFO sits)
 *
 * Given these two numbers, the formula is simple:
 *
 * RxFIFO Size = (RAM2_DEPTH * MDWIDTH / 8) - 24 - 16;
 *
 * 24 bytes is for 3x SETUP packets
 * 16 bytes is a clock domain crossing tolerance
 *
 * Given RxFIFO Size, NUMP = RxFIFOSize / 1024;
 */
static void dwc3_gadget_setup_nump(struct dwc3 *dwc)
{
	u32 ram2_depth;
	u32 mdwidth;
	u32 nump;
	u32 reg;

	ram2_depth = DWC3_GHWPARAMS7_RAM2_DEPTH(dwc->hwparams.hwparams7);
	mdwidth = DWC3_GHWPARAMS0_MDWIDTH(dwc->hwparams.hwparams0);

	nump = ((ram2_depth * mdwidth / 8) - 24 - 16) / 1024;
	nump = min_t(u32, nump, 16);

	/* update NumP */
	reg = dwc3_readl(dwc->regs, DWC3_DCFG);
	reg &= ~DWC3_DCFG_NUMP_MASK;
	reg |= nump << DWC3_DCFG_NUMP_SHIFT;
	dwc3_writel(dwc->regs, DWC3_DCFG, reg);
}

static int __dwc3_gadget_start(struct dwc3 *dwc)
{
	struct dwc3_ep *dep;
	int ret = 0;
	u32 reg;

	/*
	 * We are telling dwc3 that we want to use DCFG.NUMP as ACK TP's NUMP
	 * field instead of letting dwc3 itself calculate that automatically.
	 *
	 * This way, we maximize the chances that we'll be able to get several
	 * bursts of data without going through any sort of endpoint throttling.
	 */
	reg = dwc3_readl(dwc->regs, DWC3_GRXTHRCFG);
	if (dwc3_is_usb31(dwc))
		reg &= ~DWC31_GRXTHRCFG_PKTCNTSEL;
	else
		reg &= ~DWC3_GRXTHRCFG_PKTCNTSEL;

	dwc3_writel(dwc->regs, DWC3_GRXTHRCFG, reg);

	dwc3_gadget_setup_nump(dwc);

	/* Start with SuperSpeed Default */
	dwc3_gadget_ep0_desc.wMaxPacketSize = cpu_to_le16(512);

	dep = dwc->eps[0];
	ret = __dwc3_gadget_ep_enable(dep, DWC3_DEPCFG_ACTION_INIT);
	if (ret) {
		dev_err(dwc->dev, "failed to enable %s\n", dep->name);
		goto err0;
	}

	dep = dwc->eps[1];
	ret = __dwc3_gadget_ep_enable(dep, DWC3_DEPCFG_ACTION_INIT);
	if (ret) {
		dev_err(dwc->dev, "failed to enable %s\n", dep->name);
		goto err1;
	}

	/* begin to receive SETUP packets */
	dwc->ep0state = EP0_SETUP_PHASE;
	dwc->link_state = DWC3_LINK_STATE_SS_DIS;
	dwc3_ep0_out_start(dwc);

	dwc3_gadget_enable_irq(dwc);

	return 0;

err1:
	__dwc3_gadget_ep_disable(dwc->eps[0]);

err0:
	return ret;
}

static int dwc3_gadget_start(struct usb_gadget *g,
		struct usb_gadget_driver *driver)
{
	struct dwc3 *dwc = gadget_to_dwc(g);
	unsigned long flags;
	int ret = 0;

	//dwc3_gadget_wakeup(g);
	spin_lock_irqsave(&dwc->lock, flags);
	if (dwc->gadget_driver) {
		dev_err(dwc->dev, "%s is already bound to %s\n",
			dwc->gadget.name,
			dwc->gadget_driver->function);
		ret = -EBUSY;
		goto err1;
	}

	dwc->gadget_driver = driver;

	__dwc3_gadget_start(dwc);

	spin_unlock_irqrestore(&dwc->lock, flags);

	return 0;

err1:
	spin_unlock_irqrestore(&dwc->lock, flags);

	return ret;
}

static int dwc3_gadget_stop(struct usb_gadget *g,
			    struct usb_gadget_driver *driver)
{
	struct dwc3 *dwc = gadget_to_dwc(g);
	unsigned long flags;

	spin_lock_irqsave(&dwc->lock, flags);

	dwc3_gadget_disable_irq(dwc);
	__dwc3_gadget_ep_disable(dwc->eps[0]);
	__dwc3_gadget_ep_disable(dwc->eps[1]);

	dwc->gadget_driver	= NULL;
	spin_unlock_irqrestore(&dwc->lock, flags);

	return 0;
}

static void dwc3_gadget_set_speed(struct dwc3 *dwc,
				  enum usb_device_speed speed)
{
	unsigned long flags;
	u32 reg;

	spin_lock_irqsave(&dwc->lock, flags);
	reg = dwc3_readl(dwc->regs, DWC3_DCFG);
	reg &= ~(DWC3_DCFG_SPEED_MASK);

	/*
	 * WORKAROUND: DWC3 revision < 2.20a have an issue
	 * which would cause metastability state on Run/Stop
	 * bit if we try to force the IP to USB2-only mode.
	 *
	 * Because of that, we cannot configure the IP to any
	 * speed other than the SuperSpeed
	 *
	 * Refers to:
	 *
	 * STAR#9000525659: Clock Domain Crossing on DCTL in
	 * USB 2.0 Mode
	 */
	if (dwc->revision < DWC3_REVISION_220A &&
	    !dwc->dis_metastability_quirk) {
		reg |= DWC3_DCFG_SUPERSPEED;
	} else {
		switch (speed) {
		case USB_SPEED_LOW:
			reg |= DWC3_DCFG_LOWSPEED;
			break;
		case USB_SPEED_FULL:
			reg |= DWC3_DCFG_FULLSPEED;
			break;
		case USB_SPEED_HIGH:
			reg |= DWC3_DCFG_HIGHSPEED;
			break;
		case USB_SPEED_SUPER:
			reg |= DWC3_DCFG_SUPERSPEED;
			break;
		case USB_SPEED_SUPER_PLUS:
			if (dwc3_is_usb31(dwc))
				reg |= DWC3_DCFG_SUPERSPEED_PLUS;
			else
				reg |= DWC3_DCFG_SUPERSPEED;
			break;
		default:
			dev_err(dwc->dev, "invalid speed (%d)\n", speed);

			if (dwc->revision & DWC3_REVISION_IS_DWC31)
				reg |= DWC3_DCFG_SUPERSPEED_PLUS;
			else
				reg |= DWC3_DCFG_SUPERSPEED;
		}
	}
	dwc3_writel(dwc->regs, DWC3_DCFG, reg);

	spin_unlock_irqrestore(&dwc->lock, flags);
}

static void dwc3_gadget_poll(struct usb_gadget *g);

static const struct usb_gadget_ops dwc3_gadget_ops = {
	.get_frame		= dwc3_gadget_get_frame,
	.wakeup		= dwc3_gadget_wakeup,
	.set_selfpowered	= dwc3_gadget_set_selfpowered,
	.pullup		= dwc3_gadget_pullup,
	.udc_start		= dwc3_gadget_start,
	.udc_stop		= dwc3_gadget_stop,
	.udc_poll		= dwc3_gadget_poll,
};

/* -------------------------------------------------------------------------- */

static int dwc3_gadget_init_control_endpoint(struct dwc3_ep *dep)
{
	struct dwc3 *dwc = dep->dwc;

	usb_ep_set_maxpacket_limit(&dep->endpoint, 512);
	dep->endpoint.maxburst = 1;
	dep->endpoint.ops = &dwc3_gadget_ep0_ops;
	if (!dep->direction)
		dwc->gadget.ep0 = &dep->endpoint;

	return 0;
}

static int dwc3_gadget_init_in_endpoint(struct dwc3_ep *dep)
{
	struct dwc3 *dwc = dep->dwc;
	int mdwidth;
	int kbytes;
	int size;

	mdwidth = DWC3_MDWIDTH(dwc->hwparams.hwparams0);
	/* MDWIDTH is represented in bits, we need it in bytes */
	mdwidth /= 8;

	size = dwc3_readl(dwc->regs, DWC3_GTXFIFOSIZ(dep->number >> 1));
	if (dwc3_is_usb31(dwc))
		size = DWC31_GTXFIFOSIZ_TXFDEF(size);
	else
		size = DWC3_GTXFIFOSIZ_TXFDEF(size);

	/* FIFO Depth is in MDWDITH bytes. Multiply */
	size *= mdwidth;

	kbytes = size / 1024;
	if (kbytes == 0)
		kbytes = 1;

	/*
	 * FIFO sizes account an extra MDWIDTH * (kbytes + 1) bytes for
	 * internal overhead. We don't really know how these are used,
	 * but documentation say it exists.
	 */
	size -= mdwidth * (kbytes + 1);
	size /= kbytes;

	usb_ep_set_maxpacket_limit(&dep->endpoint, size);

	dep->endpoint.max_streams = 15;
	dep->endpoint.ops = &dwc3_gadget_ep_ops;
	list_add_tail(&dep->endpoint.ep_list,
			&dwc->gadget.ep_list);

	return dwc3_alloc_trb_pool(dep);
}

static int dwc3_gadget_init_out_endpoint(struct dwc3_ep *dep)
{
	struct dwc3 *dwc = dep->dwc;

	usb_ep_set_maxpacket_limit(&dep->endpoint, 1024);
	dep->endpoint.max_streams = 15;
	dep->endpoint.ops = &dwc3_gadget_ep_ops;
	list_add_tail(&dep->endpoint.ep_list,
			&dwc->gadget.ep_list);

	return dwc3_alloc_trb_pool(dep);
}

static int dwc3_gadget_init_endpoint(struct dwc3 *dwc, u8 epnum)
{
	struct dwc3_ep *dep;
	bool direction = epnum & 1;
	int ret;
	u8 num = epnum >> 1;

	dep = kzalloc(sizeof(*dep), GFP_KERNEL);
	if (!dep)
		return -ENOMEM;

	dep->dwc = dwc;
	dep->number = epnum;
	dep->direction = direction;
	dep->regs = dwc->regs + DWC3_DEP_BASE(epnum);
	dwc->eps[epnum] = dep;
	dep->combo_num = 0;
	dep->start_cmd_status = 0;

	snprintf(dep->name, sizeof(dep->name), "ep%u%s", num,
		 direction ? "in" : "out");

	dep->endpoint.name = dep->name;

	if (!(dep->number > 1)) {
		dep->endpoint.desc = &dwc3_gadget_ep0_desc;
		dep->endpoint.comp_desc = NULL;
	}

	spin_lock_init(&dep->lock);

	if (num == 0)
		ret = dwc3_gadget_init_control_endpoint(dep);
	else if (direction)
		ret = dwc3_gadget_init_in_endpoint(dep);
	else
		ret = dwc3_gadget_init_out_endpoint(dep);

	if (ret)
		return ret;

	INIT_LIST_HEAD(&dep->pending_list);
	INIT_LIST_HEAD(&dep->started_list);
	INIT_LIST_HEAD(&dep->cancelled_list);

	return 0;
}

static int dwc3_gadget_init_endpoints(struct dwc3 *dwc, u8 total)
{
	u8 epnum;

	INIT_LIST_HEAD(&dwc->gadget.ep_list);

	for (epnum = 0; epnum < total; epnum++) {
		int ret;

		ret = dwc3_gadget_init_endpoint(dwc, epnum);
		if (ret)
			return ret;
	}

	return 0;
}

static void dwc3_gadget_free_endpoints(struct dwc3 *dwc)
{
	struct dwc3_ep *dep;
	u8 epnum;

	for (epnum = 0; epnum < DWC3_ENDPOINTS_NUM; epnum++) {
		dep = dwc->eps[epnum];
		if (!dep)
			continue;
		/*
		 * Physical endpoints 0 and 1 are special; they form the
		 * bi-directional USB endpoint 0.
		 *
		 * For those two physical endpoints, we don't allocate a TRB
		 * pool nor do we add them the endpoints list. Due to that, we
		 * shouldn't do these two operations otherwise we would end up
		 * with all sorts of bugs when removing dwc3.ko.
		 */
		if (epnum != 0 && epnum != 1) {
			dwc3_free_trb_pool(dep);
			list_del(&dep->endpoint.ep_list);
		}

		kfree(dep);
	}
}

/* -------------------------------------------------------------------------- */

static int dwc3_gadget_ep_reclaim_completed_trb(struct dwc3_ep *dep,
						struct dwc3_request *req,
						struct dwc3_trb *trb,
						const struct dwc3_event_depevt *event,
						int status, int chain)
{
	unsigned int count;

	dwc3_ep_inc_deq(dep);

	req->num_trbs--;

	/*
	 * If we're in the middle of series of chained TRBs and we
	 * receive a short transfer along the way, DWC3 will skip
	 * through all TRBs including the last TRB in the chain (the
	 * where CHN bit is zero. DWC3 will also avoid clearing HWO
	 * bit and SW has to do it manually.
	 *
	 * We're going to do that here to avoid problems of HW trying
	 * to use bogus TRBs for transfers.
	 */
	if (chain && (trb->ctrl & DWC3_TRB_CTRL_HWO))
		trb->ctrl &= ~DWC3_TRB_CTRL_HWO;

	/*
	 * For isochronous transfers, the first TRB in a service interval must
	 * have the Isoc-First type. Track and report its interval frame number.
	 */
	if (usb_endpoint_xfer_isoc(dep->endpoint.desc) &&
	    (trb->ctrl & DWC3_TRBCTL_ISOCHRONOUS_FIRST)) {
		unsigned int frame_number;

		frame_number = DWC3_TRB_CTRL_GET_SID_SOFN(trb->ctrl);
		frame_number &= ~(dep->interval - 1);
	}

	/*
	 * If we're dealing with unaligned size OUT transfer, we will be left
	 * with one TRB pending in the ring. We need to manually clear HWO bit
	 * from that TRB.
	 */

	if (req->needs_extra_trb && !(trb->ctrl & DWC3_TRB_CTRL_CHN)) {
		trb->ctrl &= ~DWC3_TRB_CTRL_HWO;
		return 1;
	}

	count = trb->size & DWC3_TRB_SIZE_MASK;
	req->remaining += count;

	if ((trb->ctrl & DWC3_TRB_CTRL_HWO) && status != -ESHUTDOWN)
		return 1;

	if (event->status & DEPEVT_STATUS_SHORT && !chain)
		return 1;

	if (event->status & DEPEVT_STATUS_IOC)
		return 1;

	return 0;
}

static int dwc3_gadget_ep_reclaim_trb_linear(struct dwc3_ep *dep,
					     struct dwc3_request *req,
					     const struct dwc3_event_depevt *event,
					     int status)
{
	struct dwc3_trb *trb = &dep->trb_pool[dep->trb_dequeue];

	return dwc3_gadget_ep_reclaim_completed_trb(dep, req, trb,
			event, status, false);
}

static int dwc3_gadget_ep_cleanup_completed_request(struct dwc3_ep *dep,
						    const struct dwc3_event_depevt *event,
						    struct dwc3_request *req,
						    int status)
{
	int ret;

	ret = dwc3_gadget_ep_reclaim_trb_linear(dep, req, event,
						status);

	if (req->needs_extra_trb) {
		ret = dwc3_gadget_ep_reclaim_trb_linear(dep, req, event,
				status);
		req->needs_extra_trb = false;
	}

	req->request.actual = req->request.length - req->remaining;

	dwc3_gadget_giveback(dep, req, status);

	return ret;
}

static void dwc3_gadget_ep_cleanup_completed_requests(struct dwc3_ep *dep,
						      const struct dwc3_event_depevt *event,
						      int status)
{
	struct dwc3_request *req;
	struct dwc3_request *tmp;

	list_for_each_entry_safe(req, tmp, &dep->started_list, list) {
		int ret;

		ret = dwc3_gadget_ep_cleanup_completed_request(dep, event,
				req, status);
		if (ret)
			break;
	}
}

static void dwc3_gadget_endpoint_frame_from_event(struct dwc3_ep *dep,
						  const struct dwc3_event_depevt *event)
{
	dep->frame_number = event->parameters;
}

static void dwc3_gadget_endpoint_transfer_in_progress(struct dwc3_ep *dep,
						      const struct dwc3_event_depevt *event)
{
	struct dwc3 *dwc = dep->dwc;
	unsigned status = 0;
	bool stop = false;

	dwc3_gadget_endpoint_frame_from_event(dep, event);

	if (event->status & DEPEVT_STATUS_BUSERR)
		status = -ECONNRESET;

	if (event->status & DEPEVT_STATUS_MISSED_ISOC) {
		status = -EXDEV;

		if (list_empty(&dep->started_list))
			stop = true;
	}

	dwc3_gadget_ep_cleanup_completed_requests(dep, event, status);

	if (stop) {
		dwc3_stop_active_transfer(dep, true, true);
		dep->flags = DWC3_EP_ENABLED;
	}

	/*
	 * WORKAROUND: This is the 2nd half of U1/U2 -> U0 workaround.
	 * See dwc3_gadget_linksts_change_interrupt() for 1st half.
	 */
	if (dwc->revision < DWC3_REVISION_183A) {
		u32 reg;
		int i;

		for (i = 0; i < DWC3_ENDPOINTS_NUM; i++) {
			dep = dwc->eps[i];

			if (!(dep->flags & DWC3_EP_ENABLED))
				continue;

			if (!list_empty(&dep->started_list))
				return;
		}

		reg = dwc3_readl(dwc->regs, DWC3_DCTL);
		reg |= dwc->u1u2;
		dwc3_writel(dwc->regs, DWC3_DCTL, reg);

		dwc->u1u2 = 0;
	}
}

static void dwc3_gadget_endpoint_transfer_not_ready(struct dwc3_ep *dep,
						    const struct dwc3_event_depevt *event)
{
	dwc3_gadget_endpoint_frame_from_event(dep, event);
	(void) __dwc3_gadget_start_isoc(dep);
}

static void dwc3_endpoint_interrupt(struct dwc3 *dwc,
				    const struct dwc3_event_depevt *event)
{
	struct dwc3_ep *dep;
	u8 epnum = event->endpoint_number;
	u8 cmd;

	dep = dwc->eps[epnum];

	if (!(dep->flags & DWC3_EP_ENABLED)) {
		if (!(dep->flags & DWC3_EP_TRANSFER_STARTED))
			return;

		/* Handle only EPCMDCMPLT when EP disabled */
		if (event->endpoint_event != DWC3_DEPEVT_EPCMDCMPLT)
			return;
	}

	if (epnum == 0 || epnum == 1) {
		dwc3_ep0_interrupt(dwc, event);
		return;
	}

	switch (event->endpoint_event) {
	case DWC3_DEPEVT_XFERINPROGRESS:
		dwc3_gadget_endpoint_transfer_in_progress(dep, event);
		break;
	case DWC3_DEPEVT_XFERNOTREADY:
		dwc3_gadget_endpoint_transfer_not_ready(dep, event);
		break;
	case DWC3_DEPEVT_EPCMDCMPLT:
		cmd = DEPEVT_PARAMETER_CMD(event->parameters);

		if (cmd == DWC3_DEPCMD_ENDTRANSFER) {
			dep->flags &= ~DWC3_EP_TRANSFER_STARTED;
			dwc3_gadget_ep_cleanup_cancelled_requests(dep);
		}
		break;
	case DWC3_DEPEVT_STREAMEVT:
	case DWC3_DEPEVT_XFERCOMPLETE:
	case DWC3_DEPEVT_RXTXFIFOEVT:
		break;
	}
}

static void dwc3_disconnect_gadget(struct dwc3 *dwc)
{
	if (dwc->gadget_driver && dwc->gadget_driver->disconnect) {
		spin_unlock(&dwc->lock);
		dwc->gadget_driver->disconnect(&dwc->gadget);
		spin_lock(&dwc->lock);
	}
}

static void dwc3_suspend_gadget(struct dwc3 *dwc)
{
	if (dwc->gadget_driver && dwc->gadget_driver->suspend) {
		spin_unlock(&dwc->lock);
		dwc->gadget_driver->suspend(&dwc->gadget);
		spin_lock(&dwc->lock);
	}
}

static void dwc3_resume_gadget(struct dwc3 *dwc)
{
	if (dwc->gadget_driver && dwc->gadget_driver->resume) {
		spin_unlock(&dwc->lock);
		dwc->gadget_driver->resume(&dwc->gadget);
		spin_lock(&dwc->lock);
	}
}

static void dwc3_reset_gadget(struct dwc3 *dwc)
{
	if (!dwc->gadget_driver)
		return;

	if (dwc->gadget.speed != USB_SPEED_UNKNOWN) {
		spin_unlock(&dwc->lock);
		usb_gadget_udc_reset(&dwc->gadget, dwc->gadget_driver);
		spin_lock(&dwc->lock);
	}
}

static void dwc3_stop_active_transfer(struct dwc3_ep *dep, bool force,
				      bool interrupt)
{
	struct dwc3 *dwc = dep->dwc;
	struct dwc3_gadget_ep_cmd_params params;
	u32 cmd;
	int ret;

	if (!(dep->flags & DWC3_EP_TRANSFER_STARTED))
		return;

	/*
	 * NOTICE: We are violating what the Databook says about the
	 * EndTransfer command. Ideally we would _always_ wait for the
	 * EndTransfer Command Completion IRQ, but that's causing too
	 * much trouble synchronizing between us and gadget driver.
	 *
	 * We have discussed this with the IP Provider and it was
	 * suggested to giveback all requests here, but give HW some
	 * extra time to synchronize with the interconnect. We're using
	 * an arbitraty 100us delay for that.
	 *
	 * Note also that a similar handling was tested by Synopsys
	 * (thanks a lot Paul) and nothing bad has come out of it.
	 * In short, what we're doing is:
	 *
	 * - Issue EndTransfer WITH CMDIOC bit set
	 * - Wait 100us
	 */

	cmd = DWC3_DEPCMD_ENDTRANSFER;
	cmd |= force ? DWC3_DEPCMD_HIPRI_FORCERM : 0;
	cmd |= DWC3_DEPCMD_CMDIOC;
	cmd |= DWC3_DEPCMD_PARAM(dep->resource_index);
	memset(&params, 0, sizeof(params));
	ret = dwc3_send_gadget_ep_cmd(dep, cmd, &params);
	dep->resource_index = 0;

	if (dwc3_is_usb31(dwc) || dwc->revision < DWC3_REVISION_310A)
		udelay(100);
}

static void dwc3_clear_stall_all_ep(struct dwc3 *dwc)
{
	u32 epnum;

	for (epnum = 1; epnum < DWC3_ENDPOINTS_NUM; epnum++) {
		struct dwc3_ep *dep;

		dep = dwc->eps[epnum];
		if (!dep)
			continue;

		if (!(dep->flags & DWC3_EP_STALL))
			continue;

		dep->flags &= ~DWC3_EP_STALL;

		dwc3_send_clear_stall_ep_cmd(dep);
	}
}

static void dwc3_gadget_disconnect_interrupt(struct dwc3 *dwc)
{
	int reg;

	reg = dwc3_readl(dwc->regs, DWC3_DCTL);
	reg &= ~DWC3_DCTL_INITU1ENA;
	dwc3_writel(dwc->regs, DWC3_DCTL, reg);

	reg &= ~DWC3_DCTL_INITU2ENA;
	dwc3_writel(dwc->regs, DWC3_DCTL, reg);

	dwc3_disconnect_gadget(dwc);

	dwc->gadget.speed = USB_SPEED_UNKNOWN;
	dwc->setup_packet_pending = false;
	usb_gadget_set_state(&dwc->gadget, USB_STATE_NOTATTACHED);

	dwc->connected = false;
}

static void dwc3_gadget_reset_interrupt(struct dwc3 *dwc)
{
	u32 reg;

	dwc->connected = true;

	/*
	 * WORKAROUND: DWC3 revisions <1.88a have an issue which
	 * would cause a missing Disconnect Event if there's a
	 * pending Setup Packet in the FIFO.
	 *
	 * There's no suggested workaround on the official Bug
	 * report, which states that "unless the driver/application
	 * is doing any special handling of a disconnect event,
	 * there is no functional issue".
	 *
	 * Unfortunately, it turns out that we _do_ some special
	 * handling of a disconnect event, namely complete all
	 * pending transfers, notify gadget driver of the
	 * disconnection, and so on.
	 *
	 * Our suggested workaround is to follow the Disconnect
	 * Event steps here, instead, based on a setup_packet_pending
	 * flag. Such flag gets set whenever we have a SETUP_PENDING
	 * status for EP0 TRBs and gets cleared on XferComplete for the
	 * same endpoint.
	 *
	 * Refers to:
	 *
	 * STAR#9000466709: RTL: Device : Disconnect event not
	 * generated if setup packet pending in FIFO
	 */
	if (dwc->revision < DWC3_REVISION_188A) {
		if (dwc->setup_packet_pending)
			dwc3_gadget_disconnect_interrupt(dwc);
	}

	dwc3_reset_gadget(dwc);

	reg = dwc3_readl(dwc->regs, DWC3_DCTL);
	reg &= ~DWC3_DCTL_TSTCTRL_MASK;
	dwc3_writel(dwc->regs, DWC3_DCTL, reg);
	dwc->test_mode = false;
	dwc3_clear_stall_all_ep(dwc);

	/* Reset device address to zero */
	reg = dwc3_readl(dwc->regs, DWC3_DCFG);
	reg &= ~(DWC3_DCFG_DEVADDR_MASK);
	dwc3_writel(dwc->regs, DWC3_DCFG, reg);
}

static void dwc3_gadget_conndone_interrupt(struct dwc3 *dwc)
{
	struct dwc3_ep *dep;
	int ret;
	u32 reg;
	u8 speed;

	reg = dwc3_readl(dwc->regs, DWC3_DSTS);
	speed = reg & DWC3_DSTS_CONNECTSPD;
	dwc->speed = speed;

	switch (speed) {
	case DWC3_DSTS_SUPERSPEED_PLUS:
		dwc3_gadget_ep0_desc.wMaxPacketSize = cpu_to_le16(512);
		dwc->gadget.ep0->maxpacket = 512;
		dwc->gadget.speed = USB_SPEED_SUPER_PLUS;
		break;
	case DWC3_DCFG_SUPERSPEED:
		/*
		 * WORKAROUND: DWC3 revisions <1.90a have an issue which
		 * would cause a missing USB3 Reset event.
		 *
		 * In such situations, we should force a USB3 Reset
		 * event by calling our dwc3_gadget_reset_interrupt()
		 * routine.
		 *
		 * Refers to:
		 *
		 * STAR#9000483510: RTL: SS : USB3 reset event may
		 * not be generated always when the link enters poll
		 */
		if (dwc->revision < DWC3_REVISION_190A)
			dwc3_gadget_reset_interrupt(dwc);

		dwc3_gadget_ep0_desc.wMaxPacketSize = cpu_to_le16(512);
		dwc->gadget.ep0->maxpacket = 512;
		dwc->gadget.speed = USB_SPEED_SUPER;
		break;
	case DWC3_DCFG_HIGHSPEED:
		dwc3_gadget_ep0_desc.wMaxPacketSize = cpu_to_le16(64);
		dwc->gadget.ep0->maxpacket = 64;
		dwc->gadget.speed = USB_SPEED_HIGH;
		break;
	case DWC3_DCFG_FULLSPEED:
	case DWC3_DCFG_FULLSPEED1:
		dwc3_gadget_ep0_desc.wMaxPacketSize = cpu_to_le16(64);
		dwc->gadget.ep0->maxpacket = 64;
		dwc->gadget.speed = USB_SPEED_FULL;
		break;
	case DWC3_DCFG_LOWSPEED:
		dwc3_gadget_ep0_desc.wMaxPacketSize = cpu_to_le16(8);
		dwc->gadget.ep0->maxpacket = 8;
		dwc->gadget.speed = USB_SPEED_LOW;
		break;
	}

	dwc->eps[1]->endpoint.maxpacket = dwc->gadget.ep0->maxpacket;

	/* Enable USB2 LPM Capability */

	if ((dwc->revision > DWC3_REVISION_194A) &&
	    (speed != DWC3_DSTS_SUPERSPEED) &&
	    (speed != DWC3_DSTS_SUPERSPEED_PLUS)) {
		reg = dwc3_readl(dwc->regs, DWC3_DCFG);
		reg |= DWC3_DCFG_LPM_CAP;
		dwc3_writel(dwc->regs, DWC3_DCFG, reg);

		reg = dwc3_readl(dwc->regs, DWC3_DCTL);
		reg &= ~(DWC3_DCTL_HIRD_THRES_MASK | DWC3_DCTL_L1_HIBER_EN);

		reg |= DWC3_DCTL_HIRD_THRES(dwc->hird_threshold);

		/*
		 * When dwc3 revisions >= 2.40a, LPM Erratum is enabled and
		 * DCFG.LPMCap is set, core responses with an ACK and the
		 * BESL value in the LPM token is less than or equal to LPM
		 * NYET threshold.
		 */
		if (dwc->revision < DWC3_REVISION_240A && dwc->has_lpm_erratum)
			WARN(true, "LPM Erratum not available on dwc3 revisisions < 2.40a\n");

		if (dwc->has_lpm_erratum && dwc->revision >= DWC3_REVISION_240A)
			reg |= DWC3_DCTL_LPM_ERRATA(dwc->lpm_nyet_threshold);

		dwc3_writel(dwc->regs, DWC3_DCTL, reg);
	} else {
		reg = dwc3_readl(dwc->regs, DWC3_DCTL);
		reg &= ~DWC3_DCTL_HIRD_THRES_MASK;
		dwc3_writel(dwc->regs, DWC3_DCTL, reg);
	}

	dep = dwc->eps[0];
	ret = __dwc3_gadget_ep_enable(dep, DWC3_DEPCFG_ACTION_MODIFY);
	if (ret) {
		dev_err(dwc->dev, "failed to enable %s\n", dep->name);
		return;
	}

	dep = dwc->eps[1];
	ret = __dwc3_gadget_ep_enable(dep, DWC3_DEPCFG_ACTION_MODIFY);
	if (ret) {
		dev_err(dwc->dev, "failed to enable %s\n", dep->name);
		return;
	}

	/*
	 * Configure PHY via GUSB3PIPECTLn if required.
	 *
	 * Update GTXFIFOSIZn
	 *
	 * In both cases reset values should be sufficient.
	 */
}

static void dwc3_gadget_wakeup_interrupt(struct dwc3 *dwc)
{
	/*
	 * TODO take core out of low power mode when that's
	 * implemented.
	 */

	if (dwc->gadget_driver && dwc->gadget_driver->resume)
		dwc->gadget_driver->resume(&dwc->gadget);
}

static void dwc3_gadget_linksts_change_interrupt(struct dwc3 *dwc,
						 unsigned int evtinfo)
{
	enum dwc3_link_state next = evtinfo & DWC3_LINK_STATE_MASK;
	unsigned int pwropt;

	/*
	 * WORKAROUND: DWC3 < 2.50a have an issue when configured without
	 * Hibernation mode enabled which would show up when device detects
	 * host-initiated U3 exit.
	 *
	 * In that case, device will generate a Link State Change Interrupt
	 * from U3 to RESUME which is only necessary if Hibernation is
	 * configured in.
	 *
	 * There are no functional changes due to such spurious event and we
	 * just need to ignore it.
	 *
	 * Refers to:
	 *
	 * STAR#9000570034 RTL: SS Resume event generated in non-Hibernation
	 * operational mode
	 */
	pwropt = DWC3_GHWPARAMS1_EN_PWROPT(dwc->hwparams.hwparams1);
	if ((dwc->revision < DWC3_REVISION_250A) &&
	    (pwropt != DWC3_GHWPARAMS1_EN_PWROPT_HIB)) {
		if ((dwc->link_state == DWC3_LINK_STATE_U3) &&
		    (next == DWC3_LINK_STATE_RESUME)) {
			dev_dbg(dwc->dev, "ignoring transition U3 -> Resume\n");
			return;
		}
	}

	/*
	 * WORKAROUND: DWC3 Revisions <1.83a have an issue which, depending
	 * on the link partner, the USB session might do multiple entry/exit
	 * of low power states before a transfer takes place.
	 *
	 * Due to this problem, we might experience lower throughput. The
	 * suggested workaround is to disable DCTL[12:9] bits if we're
	 * transitioning from U1/U2 to U0 and enable those bits again
	 * after a transfer completes and there are no pending transfers
	 * on any of the enabled endpoints.
	 *
	 * This is the first half of that workaround.
	 *
	 * Refers to:
	 *
	 * STAR#9000446952: RTL: Device SS : if U1/U2 ->U0 takes >128us
	 * core send LGO_Ux entering U0
	 */
	if (dwc->revision < DWC3_REVISION_183A) {
		if (next == DWC3_LINK_STATE_U0) {
			u32 u1u2;
			u32 reg;

			switch (dwc->link_state) {
			case DWC3_LINK_STATE_U1:
			case DWC3_LINK_STATE_U2:
				reg = dwc3_readl(dwc->regs, DWC3_DCTL);
				u1u2 = reg & (DWC3_DCTL_INITU2ENA
						| DWC3_DCTL_ACCEPTU2ENA
						| DWC3_DCTL_INITU1ENA
						| DWC3_DCTL_ACCEPTU1ENA);

				if (!dwc->u1u2)
					dwc->u1u2 = reg & u1u2;

				reg &= ~u1u2;

				dwc3_writel(dwc->regs, DWC3_DCTL, reg);
				break;
			default:
				/* do nothing */
				break;
			}
		}
	}

	switch (next) {
	case DWC3_LINK_STATE_U1:
		if (dwc->speed == USB_SPEED_SUPER)
			dwc3_suspend_gadget(dwc);
		break;
	case DWC3_LINK_STATE_U2:
	case DWC3_LINK_STATE_U3:
		//dwc3_suspend_gadget(dwc);
		break;
	case DWC3_LINK_STATE_RESUME:
		dwc3_resume_gadget(dwc);
		break;
	default:
		/* do nothing */
		break;
	}

	dwc->link_state = next;
}

static void dwc3_gadget_suspend_interrupt(struct dwc3 *dwc,
					  unsigned int evtinfo)
{
	enum dwc3_link_state next = evtinfo & DWC3_LINK_STATE_MASK;

	if (dwc->link_state != next && next == DWC3_LINK_STATE_U3)
		dwc3_suspend_gadget(dwc);

	dwc->link_state = next;
}

static void dwc3_gadget_hibernation_interrupt(struct dwc3 *dwc,
					      unsigned int evtinfo)
{
	unsigned int is_ss = evtinfo & (1UL << 4);

	/**
	 * WORKAROUND: DWC3 revison 2.20a with hibernation support
	 * have a known issue which can cause USB CV TD.9.23 to fail
	 * randomly.
	 *
	 * Because of this issue, core could generate bogus hibernation
	 * events which SW needs to ignore.
	 *
	 * Refers to:
	 *
	 * STAR#9000546576: Device Mode Hibernation: Issue in USB 2.0
	 * Device Fallback from SuperSpeed
	 */
	if (is_ss ^ (dwc->speed == USB_SPEED_SUPER))
		return;

	/* enter hibernation here */
}

static void dwc3_gadget_interrupt(struct dwc3 *dwc,
				  const struct dwc3_event_devt *event)
{

	switch (event->type) {
	case DWC3_DEVICE_EVENT_DISCONNECT:
		dwc3_gadget_disconnect_interrupt(dwc);
		break;
	case DWC3_DEVICE_EVENT_RESET:
		dwc3_gadget_reset_interrupt(dwc);
		break;
	case DWC3_DEVICE_EVENT_CONNECT_DONE:
		dwc3_gadget_conndone_interrupt(dwc);
		break;
	case DWC3_DEVICE_EVENT_WAKEUP:
		dwc3_gadget_wakeup_interrupt(dwc);
		break;
	case DWC3_DEVICE_EVENT_HIBER_REQ:
		if (!dwc->has_hibernation) {
			WARN(1 ,"unexpected hibernation event\n");
			break;
		}
		dwc3_gadget_hibernation_interrupt(dwc, event->event_info);
		break;
	case DWC3_DEVICE_EVENT_LINK_STATUS_CHANGE:
		dwc3_gadget_linksts_change_interrupt(dwc, event->event_info);
		break;
	case DWC3_DEVICE_EVENT_EOPF:
		dev_dbg(dwc->dev, "End of Periodic Frame\n");
		/* It changed to be suspend event for version 2.30a and above */
		if (dwc->revision >= DWC3_REVISION_230A) {
			/*
			 * Ignore suspend event until the gadget enters into
			 * USB_STATE_CONFIGURED state.
			 */
			if (dwc->gadget.state >= USB_STATE_CONFIGURED)
				dwc3_gadget_suspend_interrupt(dwc,
						event->event_info);
		}
		break;
	case DWC3_DEVICE_EVENT_SOF:
		dev_dbg(dwc->dev, "Start of Periodic Frame\n");
		break;
	case DWC3_DEVICE_EVENT_ERRATIC_ERROR:
		dev_dbg(dwc->dev, "Erratic Error\n");
		break;
	case DWC3_DEVICE_EVENT_CMD_CMPL:
		dev_dbg(dwc->dev, "Command Complete\n");
		break;
	case DWC3_DEVICE_EVENT_OVERFLOW:
		dev_dbg(dwc->dev, "Overflow\n");
		break;
	default:
		dev_dbg(dwc->dev, "UNKNOWN IRQ %d\n", event->type);
	}
}

static void dwc3_process_event_entry(struct dwc3 *dwc,
				     const union dwc3_event *event)
{
	if (!event->type.is_devspec)
		dwc3_endpoint_interrupt(dwc, &event->depevt);
	else if (event->type.type == DWC3_EVENT_TYPE_DEV)
		dwc3_gadget_interrupt(dwc, &event->devt);
}

static void dwc3_gadget_poll(struct usb_gadget * g)
{
	struct dwc3 *dwc = gadget_to_dwc(g);
	struct dwc3_event_buffer *evt = dwc->ev_buf;
	u32 amount;
	u32 count;
	void *buf;
	int pos = 0;

	count = dwc3_readl(dwc->regs, DWC3_GEVNTCOUNT(0));
	count &= DWC3_GEVNTCOUNT_MASK;
	if (!count)
		return;

	buf = xzalloc(count);

	amount = min(count, evt->length - evt->lpos);
	memcpy(buf, evt->buf + evt->lpos, amount);

	if (amount < count)
		memcpy(buf + amount, evt->buf, count - amount);

	evt->lpos = (evt->lpos + count) % evt->length;

	dwc3_writel(dwc->regs, DWC3_GEVNTCOUNT(0), count);

	while (count > 0) {
		union dwc3_event event;

		event.raw = *(u32 *)(buf + pos);

		dwc3_process_event_entry(dwc, &event);

		count -= 4;
		pos += 4;
	}

	free(buf);
}

/**
 * dwc3_gadget_init - Initializes gadget related registers
 * @dwc: pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno.
 */
int dwc3_gadget_init(struct dwc3 *dwc)
{
	int ret;

	dwc->ep0_trb = dma_alloc_coherent(sizeof(*dwc->ep0_trb) * 2,
					 &dwc->ep0_trb_addr);
	if (!dwc->ep0_trb) {
		dev_err(dwc->dev, "failed to allocate ep0 trb\n");
		ret = -ENOMEM;
		goto err1;
	}

	dwc->setup_buf = xzalloc(DWC3_EP0_SETUP_SIZE);
	if (!dwc->setup_buf) {
		ret = -ENOMEM;
		goto err2;
	}

	dwc->bounce = dma_alloc_coherent(DWC3_BOUNCE_SIZE,
					&dwc->bounce_addr);
	if (!dwc->bounce) {
		dev_err(dwc->dev, "failed to allocate ep0 bounce buffer\n");
		ret = -ENOMEM;
		goto err3;
	}

	dwc->gadget.ops	= &dwc3_gadget_ops;
	dwc->gadget.max_speed	= USB_SPEED_SUPER;
	dwc->gadget.speed	= USB_SPEED_UNKNOWN;
	dwc->gadget.name	= "dwc3-gadget";

	/*
	 * Per databook, DWC3 needs buffer size to be aligned to MaxPacketSize
	 * on ep out.
	 */
	dwc->gadget.quirk_ep_out_aligned_size = true;

	if (dwc->revision < DWC3_REVISION_220A &&
	    !dwc->dis_metastability_quirk)
		dev_info(dwc->dev, "changing max_speed on rev %08x\n",
				dwc->revision);

	dwc->gadget.max_speed = dwc->maximum_speed;

	/*
	 * REVISIT: Here we should clear all pending IRQs to be
	 * sure we're starting from a well known location.
	 */

	ret = dwc3_gadget_init_endpoints(dwc, dwc->num_eps);
	if (ret)
		goto err4;

	ret = usb_add_gadget_udc((struct device_d *)dwc->dev, &dwc->gadget);
	if (ret) {
		dev_err(dwc->dev, "failed to register udc\n");
		goto err4;
	}

	dwc3_gadget_set_speed(dwc, dwc->maximum_speed);

	return 0;

err4:
	dwc3_gadget_free_endpoints(dwc);
err3:
	dma_free_coherent(dwc->bounce, 0, DWC3_BOUNCE_SIZE);

err2:
	kfree(dwc->setup_buf);

err1:
	dma_free_coherent(dwc->ep0_trb, 0, sizeof(*dwc->ep0_trb) * 2);

	return ret;
}
