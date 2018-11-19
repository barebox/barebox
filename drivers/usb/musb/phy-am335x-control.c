#include <common.h>
#include <init.h>
#include <io.h>
#include <linux/err.h>
#include <linux/spinlock.h>

#include "am35x-phy-control.h"

struct am335x_control_usb {
	struct device_d *dev;
	void __iomem *phy_reg;
	void __iomem *wkup;
	spinlock_t lock;
	struct phy_control phy_ctrl;
};

#define AM335X_USB0_CTRL		0x0
#define AM335X_USB1_CTRL		0x8
#define AM335x_USB_WKUP			0x0

#define USBPHY_CM_PWRDN		(1 << 0)
#define USBPHY_OTG_PWRDN	(1 << 1)
#define USBPHY_OTGVDET_EN	(1 << 19)
#define USBPHY_OTGSESSEND_EN	(1 << 20)

#define AM335X_PHY0_WK_EN	(1 << 0)
#define AM335X_PHY1_WK_EN	(1 << 8)

static void am335x_phy_wkup(struct  phy_control *phy_ctrl, u32 id, bool on)
{
	struct am335x_control_usb *usb_ctrl;
	u32 val;
	u32 reg;

	usb_ctrl = container_of(phy_ctrl, struct am335x_control_usb, phy_ctrl);

	switch (id) {
	case 0:
		reg = AM335X_PHY0_WK_EN;
		break;
	case 1:
		reg = AM335X_PHY1_WK_EN;
		break;
	default:
		WARN_ON(1);
		return;
	}

	spin_lock(&usb_ctrl->lock);
	val = readl(usb_ctrl->wkup);

	if (on)
		val |= reg;
	else
		val &= ~reg;

	writel(val, usb_ctrl->wkup);
	spin_unlock(&usb_ctrl->lock);
}

static void am335x_phy_power(struct phy_control *phy_ctrl, u32 id, bool on)
{
	struct am335x_control_usb *usb_ctrl;
	u32 val;
	u32 reg;

	usb_ctrl = container_of(phy_ctrl, struct am335x_control_usb, phy_ctrl);

	switch (id) {
	case 0:
		reg = AM335X_USB0_CTRL;
		break;
	case 1:
		reg = AM335X_USB1_CTRL;
		break;
	default:
		WARN_ON(1);
		return;
	}

	val = readl(usb_ctrl->phy_reg + reg);
	if (on) {
		val &= ~(USBPHY_CM_PWRDN | USBPHY_OTG_PWRDN);
		val |= USBPHY_OTGVDET_EN | USBPHY_OTGSESSEND_EN;
	} else {
		val |= USBPHY_CM_PWRDN | USBPHY_OTG_PWRDN;
	}

	writel(val, usb_ctrl->phy_reg + reg);
}

static const struct phy_control ctrl_am335x = {
	.phy_power = am335x_phy_power,
	.phy_wkup = am335x_phy_wkup,
};

static __maybe_unused struct of_device_id omap_control_usb_dt_ids[] = {
	{
		.compatible = "ti,am335x-usb-ctrl-module", .data = &ctrl_am335x
	}, {
		/* sentinel */
	},
};

struct phy_control *am335x_get_phy_control(struct device_d *dev)
{
	struct device_node *node;
	struct am335x_control_usb *ctrl_usb;

	node = of_parse_phandle(dev->device_node, "ti,ctrl_mod", 0);
	if (!node)
		return NULL;

	dev = of_find_device_by_node(node);
	if (!dev)
		return NULL;

	ctrl_usb = dev->priv;
	if (!ctrl_usb)
		return NULL;

	return &ctrl_usb->phy_ctrl;
}
EXPORT_SYMBOL(am335x_get_phy_control);


static int am335x_control_usb_probe(struct device_d *dev)
{
	struct resource *iores;
	/*struct resource	*res;*/
	struct am335x_control_usb *ctrl_usb;
	const struct phy_control *phy_ctrl;
	int ret;

	ret = dev_get_drvdata(dev, (const void **)&phy_ctrl);
	if (ret)
		return ret;

	ctrl_usb = xzalloc(sizeof(*ctrl_usb));

	ctrl_usb->dev = dev;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	ctrl_usb->phy_reg = IOMEM(iores->start);

	iores = dev_request_mem_resource(dev, 1);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	ctrl_usb->wkup = IOMEM(iores->start);

	spin_lock_init(&ctrl_usb->lock);
	ctrl_usb->phy_ctrl = *phy_ctrl;

	dev->priv = ctrl_usb;
	return 0;
};

static struct driver_d am335x_control_driver = {
	.name   = "am335x-control-usb",
	.probe  = am335x_control_usb_probe,
	.of_compatible = DRV_OF_COMPAT(omap_control_usb_dt_ids),
};
device_platform_driver(am335x_control_driver);
