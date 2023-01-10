/* SPDX-License-Identifier: GPL-2.0-only */

#include <linux/mfd/core.h>
#include <driver.h>

int mfd_add_devices(struct device *parent, const struct mfd_cell *cells,
		    int n_devs)
{
	struct device *dev;
	int ret, i;

	for (i = 0; i < n_devs; i++) {
		dev = device_alloc(cells[i].name, DEVICE_ID_DYNAMIC);
		dev->parent = parent;

		ret = device_add_data(dev, &cells[i], sizeof(cells[i]));
		if (ret)
			return ret;

		ret = platform_device_register(dev);
		if (ret)
			return ret;
	}

	return 0;
}
