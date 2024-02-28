// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <driver.h>
#include <linux/usb/usb.h>

struct otg_mode {
	struct device dev;
	unsigned int var_mode;
	unsigned int cur_mode;
	int (*set_mode_callback)(void *ctx, enum usb_dr_mode mode);
	void *ctx;
};

static int otg_set_mode(struct param_d *param, void *ctx)
{
	struct otg_mode *otg = ctx;
	int ret;

	if (otg->var_mode == USB_DR_MODE_UNKNOWN)
		return -EINVAL;

	if (otg->var_mode == otg->cur_mode)
		return 0;

	if (otg->cur_mode != USB_DR_MODE_OTG)
		return -EBUSY;

	ret = otg->set_mode_callback(otg->ctx, otg->var_mode);
	if (ret)
		return ret;

	otg->cur_mode = otg->var_mode;

	return 0;
}

static const char *otg_mode_names[] = {
	[USB_DR_MODE_UNKNOWN] = "unknown",
	[USB_DR_MODE_HOST] = "host",
	[USB_DR_MODE_PERIPHERAL] = "peripheral",
	[USB_DR_MODE_OTG] = "otg",
};

static int register_otg_device(struct device *dev, struct otg_mode *otg)
{
	struct param_d *param_mode;
	int ret;

	ret = register_device(dev);
	if (ret)
		return ret;

	param_mode = dev_add_param_enum(dev, "mode",
			otg_set_mode, NULL, &otg->var_mode,
			otg_mode_names, ARRAY_SIZE(otg_mode_names), otg);

	return PTR_ERR_OR_ZERO(param_mode);
}

static struct device otg_device = {
	.name = "otg",
	.id = DEVICE_ID_SINGLE,
};

struct bus_type otg_bus_type = {
	.name = "usbotg" /* "otg" is already taken for the alias */
};

int usb_register_otg_device(struct device *parent,
			    int (*set_mode)(void *ctx, enum usb_dr_mode mode), void *ctx)
{
	int ret;
	struct otg_mode *otg;

	otg = xzalloc(sizeof(*otg));
	otg->dev.priv = otg;
	otg->dev.parent = parent;
	otg->dev.bus = &otg_bus_type;
	otg->dev.id = DEVICE_ID_DYNAMIC;
	dev_set_name(&otg->dev, "otg");

	otg->var_mode = USB_DR_MODE_OTG;
	otg->cur_mode = USB_DR_MODE_OTG;
	otg->set_mode_callback = set_mode;
	otg->ctx = ctx;

	/* register otg.mode as an alias of otg0.mode */
	if (otg_device.parent == NULL) {
		otg_device.parent = parent;
		ret = register_otg_device(&otg_device, otg);
		if (ret)
			return ret;
	}

	return register_otg_device(&otg->dev, otg);
}

static int otg_bus_init(void)
{
	return bus_register(&otg_bus_type);
}
pure_initcall(otg_bus_init);
