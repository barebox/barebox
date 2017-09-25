#include <common.h>
#include <init.h>
#include <io.h>
#include <malloc.h>
#include <linux/err.h>
#include "am35x-phy-control.h"
#include "musb_core.h"

struct am335x_usbphy {
	void __iomem *base;
	struct phy_control *phy_ctrl;
	int id;
	struct usb_phy phy;
};

static struct am335x_usbphy *am_usbphy;

struct usb_phy *am335x_get_usb_phy(void)
{
	return &am_usbphy->phy;
}

static int am335x_init(struct usb_phy *phy)
{
	struct am335x_usbphy *am_usbphy = container_of(phy, struct am335x_usbphy, phy);

	phy_ctrl_power(am_usbphy->phy_ctrl, am_usbphy->id, true);
	return 0;
}

static int am335x_phy_probe(struct device_d *dev)
{
	struct resource *iores;
	int ret;

	am_usbphy = xzalloc(sizeof(*am_usbphy));

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		ret = PTR_ERR(iores);
		goto err_free;
	}
	am_usbphy->base = IOMEM(iores->start);

	am_usbphy->phy_ctrl = am335x_get_phy_control(dev);
	if (!am_usbphy->phy_ctrl)
		return -ENODEV;

	am_usbphy->id = of_alias_get_id(dev->device_node, "phy");
	if (am_usbphy->id < 0) {
		dev_err(dev, "Missing PHY id: %d\n", am_usbphy->id);
		return am_usbphy->id;
	}

	am_usbphy->phy.init = am335x_init;
	dev->priv = am_usbphy;

	dev_info(dev, "am_usbphy %p enabled\n", &am_usbphy->phy);

	return 0;

err_free:
	free(am_usbphy);

	return ret;
};

static __maybe_unused struct of_device_id am335x_phy_dt_ids[] = {
	{
		.compatible = "ti,am335x-usb-phy",
	}, {
		/* sentinel */
	},
};

static struct driver_d am335x_phy_driver = {
	.name   = "am335x-phy-driver",
	.probe  = am335x_phy_probe,
	.of_compatible = DRV_OF_COMPAT(am335x_phy_dt_ids),
};

static int am335x_phy_init(void)
{
	return platform_driver_register(&am335x_phy_driver);
}
fs_initcall(am335x_phy_init);
