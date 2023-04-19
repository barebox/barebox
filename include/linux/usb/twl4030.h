/*
 * Copyright (C) 2010 Michael Grzeschik <mgr@pengutronix.de>
 * Copyright (C) 2010 Sascha Hauer <sha@pengutronix.de>
 *
 * This file is released under the GPLv2
 *
 */

#ifndef __USB_TWL4030_H
#define __USB_TWL4030_H

/* Defines for bits in registers */
#define OPMODE_MASK		(3 << 3)
#define XCVRSELECT_MASK		(3 << 0)
#define CARKITMODE		(1 << 2)
#define OTG_ENAB		(1 << 5)
#define PHYPWD			(1 << 0)
#define CLOCKGATING_EN		(1 << 2)
#define CLK32K_EN		(1 << 1)
#define REQ_PHY_DPLL_CLK	(1 << 0)
#define PHY_DPLL_CLK		(1 << 0)

/*
 * USB
 */
int twl4030_usb_ulpi_init(void);

#endif /* __USB_TWL4030_H */
