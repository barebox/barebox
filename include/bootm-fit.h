/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __BOOTM_FIT_H
#define __BOOTM_FIT_H

#include <linux/types.h>
#include <image-fit.h>
#include <bootm.h>

struct resource;

#ifdef CONFIG_BOOTM_FITIMAGE

int bootm_load_fit_os(struct image_data *data, unsigned long load_address);

struct resource *bootm_load_fit_initrd(struct image_data *data,
				       unsigned long load_address);

void *bootm_get_fit_devicetree(struct image_data *data);

int bootm_open_fit(struct image_data *data);

static inline void bootm_close_fit(struct image_data *data)
{
	fit_close(data->os_fit);
}

static inline bool bootm_fit_has_fdt(struct image_data *data)
{
	if (!data->os_fit)
		return false;

	return fit_has_image(data->os_fit, data->fit_config, "fdt");
}

#else

static inline int bootm_load_fit_os(struct image_data *data,
				    unsigned long load_address)
{
	return -ENOSYS;
}

static inline struct resource *bootm_load_fit_initrd(struct image_data *data,
						     unsigned long load_address)
{
	return ERR_PTR(-ENOSYS);
}

static inline void *bootm_get_fit_devicetree(struct image_data *data)
{
	return ERR_PTR(-ENOSYS);
}

static inline int bootm_open_fit(struct image_data *data)
{
	return -ENOSYS;
}

static inline void bootm_close_fit(struct image_data *data)
{
}

static inline bool bootm_fit_has_fdt(struct image_data *data)
{
	return false;
}

#endif

#endif
