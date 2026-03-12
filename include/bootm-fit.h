/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __BOOTM_FIT_H
#define __BOOTM_FIT_H

#include <linux/types.h>
#include <image-fit.h>
#include <linux/errno.h>

struct image_data;

#ifdef CONFIG_BOOTM_FITIMAGE

int bootm_open_fit(struct image_data *data, bool override);

#else

static inline int bootm_open_fit(struct image_data *data, bool override)
{
	return -ENOSYS;
}

#endif

#endif
