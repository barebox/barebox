// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <driver.h>
#include <usb/usb.h>

static int (*set_mode_callback)(void *ctx, enum usb_dr_mode mode);
static unsigned int otg_mode;

static int otg_set_mode(struct param_d *param, void *ctx)
{
	static int cur_mode = USB_DR_MODE_OTG;
	int ret;

	if (otg_mode == USB_DR_MODE_UNKNOWN)
		return -EINVAL;

	if (otg_mode == cur_mode)
		return 0;

	if (cur_mode != USB_DR_MODE_OTG)
		return -EBUSY;

	ret = set_mode_callback(ctx, otg_mode);
	if (ret)
		return ret;

	cur_mode = otg_mode;

	return 0;
}

static const char *otg_mode_names[] = {
	[USB_DR_MODE_UNKNOWN] = "unknown",
	[USB_DR_MODE_HOST] = "host",
	[USB_DR_MODE_PERIPHERAL] = "peripheral",
	[USB_DR_MODE_OTG] = "otg",
};

static struct device_d otg_device = {
	.name = "otg",
	.id = DEVICE_ID_SINGLE,
};

int usb_register_otg_device(struct device_d *parent,
			    int (*set_mode)(void *ctx, enum usb_dr_mode mode), void *ctx)
{
	int ret;
	struct param_d *param_mode;

	if (otg_device.parent)
		return -EBUSY;

	otg_device.parent = parent;
	set_mode_callback = set_mode;
	otg_mode = USB_DR_MODE_OTG;

	ret = register_device(&otg_device);
	if (ret)
		return ret;

	param_mode = dev_add_param_enum(&otg_device, "mode",
			otg_set_mode, NULL, &otg_mode,
			otg_mode_names, ARRAY_SIZE(otg_mode_names), ctx);
	if (IS_ERR(param_mode))
		return PTR_ERR(param_mode);

	return 0;
}
