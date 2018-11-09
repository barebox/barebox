#ifndef __MACH_MXS_DEVICES_H
#define __MACH_MXS_DEVICES_H

#include <common.h>
#include <linux/sizes.h>
#include <xfuncs.h>
#include <driver.h>
#include <mach/imx-regs.h>

static inline struct device_d *mxs_add_nand(unsigned long gpmi_base, unsigned long bch_base)
{
	struct resource res[] = {
		{
			.start = gpmi_base,
			.end = gpmi_base + SZ_8K - 1,
			.flags = IORESOURCE_MEM,
		}, {
			.start = bch_base,
			.end = bch_base + SZ_8K - 1,
			.flags = IORESOURCE_MEM,
		},
	};

	struct device_d *dev = xzalloc(sizeof(*dev));

	dev->resource = xzalloc(sizeof(struct resource) * ARRAY_SIZE(res));
	memcpy(dev->resource, res, sizeof(struct resource) * ARRAY_SIZE(res));
	dev->num_resources = ARRAY_SIZE(res);
	dev_set_name(dev, "mxs_nand");
	dev->id = DEVICE_ID_DYNAMIC;

	platform_device_register(dev);

	return dev;
};

static inline struct device_d *imx23_add_nand(void)
{
	return mxs_add_nand(MXS_GPMI_BASE, MXS_BCH_BASE);
}

static inline struct device_d *imx28_add_nand(void)
{
	return mxs_add_nand(MXS_GPMI_BASE, MXS_BCH_BASE);
}

#endif /* __MACH_MXS_DEVICES_H */
