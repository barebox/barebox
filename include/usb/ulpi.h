#ifndef __MACH_ULPI_H
#define __MACH_ULPI_H

int ulpi_write(u8 bits, int reg, void __iomem *view);
int ulpi_set(u8 bits, int reg, void __iomem *view);
int ulpi_clear(u8 bits, int reg, void __iomem *view);
int ulpi_read(int reg, void __iomem *view);
int ulpi_setup(void __iomem *view, int on);

/* ULPI register addresses */
#define ULPI_VID_LOW		0x00	/* Vendor ID low */
#define ULPI_VID_HIGH		0x01	/* Vendor ID high */
#define ULPI_PID_LOW		0x02	/* Product ID low */
#define ULPI_PID_HIGH		0x03	/* Product ID high */
#define ULPI_FUNCTION_CTRL	0x04
#define ULPI_ITFCTL		0x07	/* Interface Control */
#define ULPI_OTGCTL		0x0A	/* OTG Control */

/* add to above register address to access Set/Clear functions */
#define ULPI_REG_SET		0x01
#define ULPI_REG_CLEAR		0x02

/* Function Control */
#define ULPI_FC_XCVRSEL_MASK		(3 << 0)
#define ULPI_FC_HIGH_SPEED		(0 << 0)
#define ULPI_FC_FULL_SPEED		(1 << 0)
#define ULPI_FC_LOW_SPEED		(2 << 0)
#define ULPI_FC_FS4LS			(3 << 0)
#define ULPI_FC_TERMSELECT		(1 << 2)
#define ULPI_FC_OPMODE_MASK		(3 << 3)
#define ULPI_FC_OPMODE_NORMAL		(0 << 3)
#define ULPI_FC_OPMODE_NONDRIVING	(1 << 3)
#define ULPI_FC_OPMODE_DISABLE_NRZI	(2 << 3)
#define ULPI_FC_OPMODE_NOSYNC_NOEOP	(3 << 3)
#define ULPI_FC_RESET			(1 << 5)
#define ULPI_FC_SUSPENDM		(1 << 6)

/* Interface Control */
#define ULPI_IFACE_6_PIN_SERIAL_MODE	(1 << 0)
#define ULPI_IFACE_3_PIN_SERIAL_MODE	(1 << 1)
#define ULPI_IFACE_CARKITMODE		(1 << 2)
#define ULPI_IFACE_CLOCKSUSPENDM	(1 << 3)
#define ULPI_IFACE_AUTORESUME		(1 << 4)
#define ULPI_IFACE_EXTVBUS_COMPLEMENT	(1 << 5)
#define ULPI_IFACE_PASSTHRU		(1 << 6)
#define ULPI_IFACE_PROTECT_IFC_DISABLE	(1 << 7)

/* ULPI OTG Control Register bits */
#define ULPI_OTG_USE_EXT_VBUS_IND	(1 << 7)	/* Use ext. Vbus indicator */
#define ULPI_OTG_DRV_VBUS_EXT		(1 << 6)	/* Drive Vbus external */
#define ULPI_OTG_DRV_VBUS		(1 << 5)	/* Drive Vbus */
#define ULPI_OTG_CHRG_VBUS		(1 << 4)	/* Charge Vbus */
#define ULPI_OTG_DISCHRG_VBUS		(1 << 3)	/* Discharge Vbus */
#define ULPI_OTG_DM_PULL_DOWN		(1 << 2)	/* enable DM Pull Down */
#define ULPI_OTG_DP_PULL_DOWN		(1 << 1)	/* enable DP Pull Down */
#define ULPI_OTG_ID_PULL_UP		(1 << 0)	/* enable ID Pull Up */

#endif /* __MACH_ULPI_H */

