/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Common board code functions WolfVision boards.
 *
 * Copyright (C) 2024 WolfVision GmbH.
 */

#ifndef _BOARDS_WOLFVISION_COMMON_H
#define _BOARDS_WOLFVISION_COMMON_H

#define WV_RK3568_HWID_MAX 17

struct wv_overlay {
	const char *name;
	const char *filename;
	const void *data;
};

struct wv_rk3568_extension {
	int adc_chan;
	const char *name;
	const struct wv_overlay overlays[WV_RK3568_HWID_MAX];
};

int wolfvision_apply_overlay(const struct wv_overlay *overlay, char **files);

int wolfvision_register_ethaddr(void);

int wolfvision_rk3568_detect_hw(const struct wv_rk3568_extension *extensions,
				int num_extensions, char **overlays);

#endif /* _BOARDS_WOLFVISION_COMMON_H */
