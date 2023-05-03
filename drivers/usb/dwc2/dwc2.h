/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <linux/usb/usb.h>
#include <linux/usb/usb_defs.h>
#include <linux/usb/gadget.h>
#include <linux/phy/phy.h>

#include "regs.h"
#include "core.h"

/* Core functions */
void dwc2_set_default_params(struct dwc2 *dwc2);
void dwc2_get_device_properties(struct dwc2 *dwc2);
int dwc2_check_core_version(struct dwc2 *dwc2);
void dwc2_get_hwparams(struct dwc2 *dwc2);

void dwc2_init_fs_ls_pclk_sel(struct dwc2 *dwc2);
void dwc2_flush_all_fifo(struct dwc2 *dwc2);
void dwc2_flush_tx_fifo(struct dwc2 *dwc2, const int idx);
int dwc2_tx_fifo_count(struct dwc2 *dwc2);

int dwc2_phy_init(struct dwc2 *dwc2, bool select_phy);
int dwc2_gahbcfg_init(struct dwc2 *dwc2);
void dwc2_gusbcfg_init(struct dwc2 *dwc2);
int dwc2_get_dr_mode(struct dwc2 *dwc2);

int dwc2_core_reset(struct dwc2 *dwc2);
void dwc2_core_init(struct dwc2 *dwc2);

void dwc2_wait_for_mode(struct dwc2 *dwc2, bool host_mode);
bool dwc2_iddig_filter_enabled(struct dwc2 *dwc2);

/* Host functions */
#ifdef CONFIG_USB_DWC2_HOST
int dwc2_submit_roothub(struct dwc2 *dwc2, struct usb_device *dev,
		unsigned long pipe, void *buf, int len,
		struct devrequest *setup);
int dwc2_register_host(struct dwc2 *dwc2);
void dwc2_host_uninit(struct dwc2 *dwc2);
#else
static inline int dwc2_register_host(struct dwc2 *dwc2) { return -ENODEV; }
static inline void dwc2_host_uninit(struct dwc2 *dwc2) {};
#endif

/* Gadget functions */
#ifdef CONFIG_USB_DWC2_GADGET
int dwc2_gadget_init(struct dwc2 *dwc2);
void dwc2_gadget_uninit(struct dwc2 *dwc2);
#else
static inline int dwc2_gadget_init(struct dwc2 *dwc2) { return -ENODEV; }
static inline void dwc2_gadget_uninit(struct dwc2 *dwc2) {};
#endif
