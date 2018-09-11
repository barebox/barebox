#ifndef __MACH_USB_H_
#define __MACH_USB_H_

/* configuration bits for i.MX25 and i.MX35 */
#define MX35_H1_SIC_SHIFT	21
#define MX35_H1_SIC_MASK	(0x3 << MX35_H1_SIC_SHIFT)
#define MX35_H1_PM_BIT		(1 << 16)
#define MX35_H1_IPPUE_UP_BIT	(1 << 7)
#define MX35_H1_IPPUE_DOWN_BIT	(1 << 6)
#define MX35_H1_TLL_BIT		(1 << 5)
#define MX35_H1_USBTE_BIT	(1 << 4)
#define MXC_EHCI_INTERFACE_SINGLE_UNI	(2 << 0)

int imx6_usb_phy2_disable_oc(void);
int imx6_usb_phy2_enable(void);

#define USBCMD	0x140
#define USB_CMD_RESET	0x00000002

/*
 * imx_reset_otg_controller - reset the USB OTG controller
 * @base: The base address of the controller
 *
 * When booting from USB the ROM just leaves the controller enabled. This can
 * have bad side effects when for example we change PLL frequencies. In this
 * case it is seen that the hub the board is connected to gets confused and USB
 * is no longer working properly on the remote host. This function resets the
 * OTG controller. It should be called before the clocks the controller hangs on
 * is fiddled with.
 */
static inline void imx_reset_otg_controller(void __iomem *base)
{
	u32 r;

	r = readl(base + USBCMD);
	r |= USB_CMD_RESET;
	writel(r, base + USBCMD);
}

#endif /* __MACH_USB_H_*/
