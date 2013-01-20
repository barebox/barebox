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

#endif /* __MACH_USB_H_*/
